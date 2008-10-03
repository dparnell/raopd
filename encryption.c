/* 
Copyright 2008, David Allan

This file is part of raopd.

raopd is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your
option) any later version.

raopd is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with raopd.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/engine.h>
#include <openssl/bn.h>

#include <arpa/inet.h>

#include <faac.h>
#include <faaccfg.h>

#include "lt.h"
#include "utility.h"
#include "syscalls.h"
#include "rtsp.h"
#include "encryption.h"
#include "encoding.h"

#define DEFAULT_FACILITY LT_ENCRYPTION


utility_retcode_t generate_aes_iv(struct aes_data *aes_data)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	DEBG("Generating a %d byte initialization vector\n", sizeof(aes_data->iv));

	ret = get_random_bytes((uint8_t *)&aes_data->iv, sizeof(aes_data->iv));

	FUNC_RETURN;
	return ret;
}


utility_retcode_t generate_aes_key(struct aes_data *aes_data)
{
	utility_retcode_t ret;

	FUNC_ENTER;

	DEBG("Generating a %d byte AES key\n", sizeof(aes_data->key));

	ret = get_random_bytes((uint8_t *)&aes_data->key, sizeof(aes_data->key));

	FUNC_RETURN;
	return ret;
}


int raopd_rsa_encrypt_openssl(uint8_t *text, int len, uint8_t *res)
{
	RSA *rsa;
	uint8_t modulo[300];
	uint8_t exponent[24];
	size_t size;

        char n[] = 
            "59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUtwC"
            "5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDR"
            "KSKv6kDqnw4UwPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuB"
            "OitnZ/bDzPHrTOZz0Dew0uowxf/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJ"
            "Q+87X6oV3eaYvt3zWZYD6z5vYTcrtij2VZ9Zmni/UAaHqn9JdsBWLUEpVviYnh"
            "imNVvYFZeCXg/IdTQ+x4IRdiXNv5hEew==";

        char e[] = "AQAB";

	rsa = RSA_new();
	raopd_base64_decode(modulo, sizeof(modulo), n, syscalls_strlen(n), &size);
	rsa->n = BN_bin2bn(modulo, size, NULL);
	raopd_base64_decode(exponent, sizeof(exponent), e, syscalls_strlen(e), &size);
	rsa->e = BN_bin2bn(exponent, size, NULL);
	size = RSA_public_encrypt(len, text, res, rsa, RSA_PKCS1_OAEP_PADDING);
	RSA_free(rsa);

	DEBG("RSA encrypted result (%d bytes)\n", size);

	return size;
}


struct transmit_buffer {
	uint8_t header[16];
	uint8_t data[];
};

#define AUDIO_BUFLEN 4 * 1024
#define ENCRYPTED_BUFLEN 8 * 1024
#define TRANSMIT_BUFLEN 16 * 1024
#define PCM_READ_SIZE 4096
#define PCM_BYTES_PER_SAMPLE 2

static faacEncHandle open_faac_encoder(unsigned long *num_input_samples,
				       unsigned long *max_output_bytes)
{
	faacEncHandle encoder_handle;
	faacEncConfigurationPtr format;

	encoder_handle = faacEncOpen(44100,
				     2,
				     num_input_samples,
				     max_output_bytes);

	INFO("Encoder opened (num_input_samples: %lu "
	     "max_output_bytes: %lu)\n",
	     *num_input_samples,
	     *max_output_bytes);

	format = faacEncGetCurrentConfiguration(encoder_handle);

	INFO("Got current configuration\n");

	format->aacObjectType = LOW;
	format->mpegVersion = MPEG4;
	format->useTns = 0; /* Temporal Noise Shaping, 0 is default */
	format->allowMidside = 1;
	format->outputFormat = 1; /* ADTS: Audio Data Transport Stream */
	format->bandWidth = 0;
	format->inputFormat = FAAC_INPUT_16BIT;

	faacEncSetConfiguration(encoder_handle, format);

	INFO("Set configuration parameters\n");

	return encoder_handle;
}


static int read_aex(int session_fd)
{
	uint8_t buf[256];
	uint32_t *aex_data;
	int rsize;
	int ret = 0;
	long arg;

	DEBG("Before read from AEX\n");

	if ((arg = fcntl(session_fd, F_GETFL, NULL)) < 0) { 
		ERRR("Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
		ret = 1;
		goto out;
	}

	arg |= O_NONBLOCK; 

	if (fcntl(session_fd, F_SETFL, arg) < 0) { 
		ERRR("Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
		ret = 1;
		goto out;
	}

	ret = syscalls_read(session_fd, buf, sizeof(buf));

	if (0 < ret) {
		DEBG("Read %d bytes from AEX\n", ret);
		aex_data = (uint32_t *)(buf + 0x2c);
		rsize = ntohl(*aex_data);
		DEBG("rsize: %d\n", rsize);
	}

	if (0 > ret) {
		if (EAGAIN == errno) {
			DEBG("Got EAGAIN from read\n");
		} else {
			DEBG("Failure reading from server: %s\n", strerror(errno));
			ret = 1;
		}
	}

	if (0 == ret) {
		DEBG("Server closed connection\n");
		ret = 1;
	}

out:
	return ret;
}

utility_retcode_t write_encrypted_data(int data_fd,
				       int session_fd,
				       struct aes_data *aes_data)
{
	utility_retcode_t ret = UTILITY_SUCCESS;
	uint8_t *audio_buf;
	uint8_t *encrypted_buf;
	struct transmit_buffer *transmit_buf;
	int reported_len, encrypted_len, transmit_len;
	EVP_CIPHER_CTX ctx;
	int bytes_written, total_written = 0;
	uint8_t header[] = {
		0x24, 0x00, 0x00, 0x00,
		0xF0, 0xFF, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
        };
	faacEncHandle encoder_handle;
	int done = 0;
	unsigned long num_input_samples, max_output_bytes;
	int bytes_encoded, num_samples_read;
	uint8_t *pcm_buf, *encoded_buf;
	size_t pcm_bytes_read;

	FUNC_ENTER;

	audio_buf = syscalls_malloc(AUDIO_BUFLEN);
	encrypted_buf = syscalls_malloc(ENCRYPTED_BUFLEN);
	transmit_buf = syscalls_malloc(TRANSMIT_BUFLEN);

	EVP_CIPHER_CTX_init(&ctx);
	EVP_CipherInit_ex(&ctx,
			  EVP_aes_256_cbc(),
			  NULL,
			  aes_data->key,
			  aes_data->iv,
			  1 /* encrypt */);

	encoder_handle = open_faac_encoder(&num_input_samples, &max_output_bytes);

	pcm_buf = malloc(32 * 1024);
	encoded_buf = malloc(32 * 1024);

	bytes_written = 0;

        while (1 != done) {

		DEBG("Attempting to read from PCM data file\n");

		pcm_bytes_read = syscalls_read(data_fd,
					       pcm_buf,
					       num_input_samples * PCM_BYTES_PER_SAMPLE);

		num_samples_read = pcm_bytes_read / PCM_BYTES_PER_SAMPLE;

		DEBG("Read %d bytes of pcm data (%d samples); calling encode\n",
		     pcm_bytes_read, num_samples_read);

		bytes_encoded = faacEncEncode(encoder_handle,
					      (int32_t *)pcm_buf,
					      num_samples_read,
					      encoded_buf,
					      max_output_bytes);

		DEBG("Encoded %d bytes\n", bytes_encoded);

		if (bytes_encoded > 0) {
			DEBG("calling cipher update\n");

			EVP_CipherUpdate(&ctx,
					 encrypted_buf,
					 &encrypted_len,
					 encoded_buf,
					 bytes_encoded);

			DEBG("after cipher update (encrypted_len: %d)\n",
			     encrypted_len);

			syscalls_memcpy(transmit_buf->header,
					header,
					sizeof(header));
			syscalls_memcpy(transmit_buf->data,
					encrypted_buf,
					encrypted_len);

			transmit_len = encrypted_len + sizeof(header);

			/* The calculation of length to put into the
			 * header is taken from the raop_play and
			 * JustePort code.  It's not clear why the
			 * reported length in the header is 4 bytes
			 * less than the actual length.  */
			reported_len = bytes_encoded + sizeof(header) - 4;
			DEBG("reported_len: %d\n", reported_len);

			transmit_buf->header[2] = reported_len >> 8;
			transmit_buf->header[3] = reported_len & 0xff;

			bytes_written = syscalls_write(session_fd, transmit_buf, transmit_len);
			if (bytes_written >= 0) {
				total_written += bytes_written;
				DEBG("after write (total written: %d)\n", total_written);
			} else {
				ERRR("write failed: %d\n", bytes_written);
				break;
			}

			if (1 == read_aex(session_fd)) {
				ERRR("Error reading from AEX\n");
				break;
			}
		}

		done = (0 == num_samples_read && 0 == bytes_encoded) ? 1 : 0;
	}

	printf("Closing encoder handle\n");

	faacEncClose(encoder_handle);

	EVP_CipherFinal_ex(&ctx, encrypted_buf, &encrypted_len); 
	EVP_CIPHER_CTX_cleanup(&ctx);

	syscalls_write(session_fd, encrypted_buf, encrypted_len); 

	FUNC_RETURN;
	return ret;
}
