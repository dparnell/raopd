/* 
Copyright (C) 2004 Shiro Ninomiya <shiron@snino.com>
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

#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>

#include "syscalls.h"
#include "utility.h"
#include "lt.h"
#include "rtsp.h"
#include "raop_play_send_audio.h"
#include "audio_debug.h"

#define DEFAULT_FACILITY LT_RAOP_PLAY_SEND_AUDIO

static raopld_t *raopld;

/*
 * if newsize < 4096, align the size to power of 2
 */
static inline int realloc_memory(void **p, int newsize, const char *func)
{
	void *np;
	int i=0;
	int n=16;
	for(i=0;i<8;i++){
		if(newsize<=n){
			newsize=n;
			break;
		}
		n=n<<1;
	}
	newsize=newsize;
	np=realloc(*p,newsize);
	if(!np){
		ERRR("%s: realloc failed: %s\n",func,strerror(errno));
		return -1;
	}
	*p=np;
	return 0;
}



static int pcm_close(auds_t *auds)
{
	pcm_t *pcm=(pcm_t *)auds->stream;
	if(!pcm) return -1;
	if(pcm->dfd>=0) syscalls_close(pcm->dfd);
	if(pcm->buffer) syscalls_free(pcm->buffer);
	syscalls_free(pcm);
	auds->stream=NULL;
	return 0;
}


/* write bits filed data, *bpos=0 for msb, *bpos=7 for lsb
   d=data, blen=length of bits field
 */
static inline void bits_write(uint8_t **p, uint8_t d, int blen, int *bpos)
{
	int8_t lb, rb, bd;
	int dval;

	/* leftmost bit is 7 - position */
	lb = 7 - *bpos;
	/* rightmost bit is left bit - length - 1 */
	rb = lb - blen + 1;

	dval = (int)d;
	DEBG("**p: 0x%x d: 0x%x blen: %d *bpos: %d\n", **p, dval, blen, *bpos);

	if (rb >= 0) {
		bd = d << rb;
		if (*bpos) {
			**p |= bd;
		} else {
			**p = bd;
		}
		*bpos += blen;
	} else {
		bd = d >> (-rb);
		**p |= bd;
		*p += 1;
		**p = d << (8 + rb);
		*bpos = (-rb);
	}
}

static int auds_write_pcm(uint8_t *buffer, uint8_t **data, int *size,
			  int bsize, data_source_t *ds)
{
	uint8_t one[4];
	int count=0;
	int bpos=0;
	uint8_t *bp=buffer;
	int i,nodata=0;
	static uint8_t *orig_bp = NULL;

	ERRR("Manipulating PCM data (size: %d bsize: %d)\n", *size, bsize);

	/* This looks like a header for the data block.  I would think
	 * it would be fairly easy to create a struct for the
	 * bitfields and set them that way, rather than bit by bit. */
	orig_bp = bp;

	bits_write(&bp,1,3,&bpos); // channel=1, stereo
	bits_write(&bp,0,4,&bpos); // unknown
	bits_write(&bp,0,8,&bpos); // unknown
	bits_write(&bp,0,4,&bpos); // unknown
	bits_write(&bp,0,1,&bpos); // hassize
	bits_write(&bp,0,2,&bpos); // unused
	bits_write(&bp,1,1,&bpos); // is-not-compressed

	ERRR("orig_bp: %p bp: %p bp - orig_bp: 0x%x orig_bp as hex: 0x%x bpos: %d\n",
	     (void *)orig_bp,
	     (void *)bp,
	     (int)(bp - orig_bp),
	     (short)*orig_bp,
	     bpos);

	while (1) {
		if (ds->u.mem.size <= count*4) {
			nodata = 1;
		}

		*((int16_t*)one)=ds->u.mem.data[count*2];
		*((int16_t*)one+1)=ds->u.mem.data[count*2+1];

		if (nodata) {
			break;
		}

		/* Is this code just reversing the byte order of the
		 * PCM data, i.e., on Intel architectures, converting
		 * it to network byte order?  */
		bits_write(&bp,one[1],8,&bpos);
		bits_write(&bp,one[0],8,&bpos);
		bits_write(&bp,one[3],8,&bpos);
		bits_write(&bp,one[2],8,&bpos);

		if (++count == bsize) {
			break;
		}
	}

	if (!count) {
		/* when no data at all, it should stop playing */
		return -1;
	}

	/* when readable size is less than bsize, fill 0 at the bottom */
	for (i = 0 ; i < ((bsize - count) * 4) ; i++) {
		bits_write(&bp,0,8,&bpos);
	}

	*size = (bp - buffer);
	if (bpos) {
		*size += 1;
	}

	dump_converted(buffer, *size);

	INFO("Converted length %d\n", *size);

	*data = buffer;
	return 0;
}


static int aud_clac_chunk_size(int sample_rate)
{
	int bsize=MAX_SAMPLES_IN_CHUNK;
	int ratio=DEFAULT_SAMPLE_RATE*100/sample_rate;
	// to make suer the resampled size is <= 4096
	if(ratio>100) bsize=bsize*100/ratio-1;

	INFO("Audio chunk size: %d\n", bsize);

	return bsize;
}


static int pcm_open(auds_t *auds, char *fname)
{
	pcm_t *pcm=malloc(sizeof(pcm_t));
	if(!pcm) return -1;
	memset(pcm,0,sizeof(pcm_t));
	auds->stream=(void *)pcm;
	pcm->dfd=open(fname, O_RDONLY|O_NONBLOCK);
	if(pcm->dfd<0) goto erexit;
	auds->sample_rate=DEFAULT_SAMPLE_RATE;
	auds->chunk_size=aud_clac_chunk_size(auds->sample_rate);
	pcm->buffer=(__u8 *)malloc(MAX_SAMPLES_IN_CHUNK*4+16);
	if(!pcm->buffer) goto erexit;
	return 0;

erexit:
	pcm_close(auds);
	return -1;
}


static int auds_close(auds_t *auds)
{
	if(auds->stream){
		pcm_close(auds);
	}

	free(auds);
	return 0;
}


static auds_t *auds_open(char *fname, data_type_t adt)
{
	int rval=-1;
	auds_t *auds;

	auds = syscalls_malloc(sizeof(auds_t));
	if (!auds) return NULL;
	syscalls_memset(auds, 0, sizeof(auds_t));

	INFO("Opening audio file \"%s\"\n", fname);

	auds->channels=2; //default is stereo
	auds->data_type=adt;

	rval = pcm_open(auds, fname);

	if (rval) {
		ERRR("return value from xxx_open: %d\n", rval);
		goto erexit;
	}

	return auds;

erexit:
	ERRR("errror: %s\n",__func__);
	auds_close(auds);
	return NULL;
}


static int pcm_get_next_sample(auds_t *auds, uint8_t **data, int *size)
{
	int rval = 0;
	pcm_t *pcm=(pcm_t *)auds->stream;
	int bsize = auds->chunk_size;
	data_source_t ds={.type=MEMORY};
	uint8_t *rbuf=NULL;
	size_t bytes_read;
	static int total_read = 0;

	INFO("Preparing to get next PCM audio sample (fd: %d\n", pcm->dfd);

	if(!(rbuf=(uint8_t*)malloc(bsize*4))) return -1;
	memset(rbuf,0,bsize*4);
	/* ds.u.mem.size, ds.u.mem.data */
	bytes_read = syscalls_read(pcm->dfd,rbuf,bsize*4);
	ds.u.mem.size = bytes_read;
	total_read += bytes_read;

	INFO("Read %d bytes from PCM audio file (total: %d)\n",
	     (int)bytes_read, total_read);

	if (bytes_read == 0){
		INFO("End of audio file\n");
		return -1;
	}

	if (ds.u.mem.size < 0){
		ERRR("%s: data read error(%s)\n", __func__, strerror(errno));
		return -1;
	}

	dump_raw_pcm(rbuf, bytes_read);

	if (ds.u.mem.size < MINIMUM_SAMPLE_SIZE * 4) {
		ds.u.mem.size = MINIMUM_SAMPLE_SIZE * 4;
	}

	ds.u.mem.data=(int16_t*)rbuf;
	bsize=ds.u.mem.size/4;

	ERRR("size: %d\n", *size);
	/* ds is a local that gets rbuf as one of its fields; output
	 * of auds_write_pcm goes to pcm->buffer */
	rval=auds_write_pcm(pcm->buffer, data, size, bsize, &ds);

	free(rbuf);
	return rval;
}


static int set_fd_event(int fd, int flags, fd_callback_t cbf, void *p)
{
	int i;

	INFO("fd: %d flags: 0x%x\n", fd, flags);

	// check the same fd first. if it exists, update it
	for(i=0;i<MAX_NUM_OF_FDS;i++){
		if(raopld->fds[i].fd==fd){
			raopld->fds[i].dp=p;
			raopld->fds[i].cbf=cbf;
			raopld->fds[i].flags=flags;
			return 0;
		}
	}
	// then create a new one
	for(i=0;i<MAX_NUM_OF_FDS;i++){
		if(raopld->fds[i].fd<0){
			raopld->fds[i].fd=fd;
			raopld->fds[i].dp=p;
			raopld->fds[i].cbf=cbf;
			raopld->fds[i].flags=flags;
			return 0;
		}
	}
	return -1;
}


static int fd_event_callback(void *p, int flags)
{
	int i;
	uint8_t buf[256];
	raopcl_data_t *raopcld;
	int rsize;
	if(!p) return -1;
	raopcld=(raopcl_data_t *)p;

	INFO("flags: 0x%x\n", flags);

	if (flags & RAOP_FD_READ) {
		i=syscalls_read(raopcld->sfd,buf,sizeof(buf));

		INFO("read from %d returned %d\n", raopcld->sfd, i);

		if(i>0){
			rsize=GET_BIGENDIAN_INT(buf+0x2c);

			INFO("rsize: %d\n", rsize);

			raopcld->size_in_aex=rsize;
			syscalls_gettimeofday(&raopcld->last_read_tv,NULL);
			ERRR("%s: read %d bytes, rsize=%d\n", __func__, i,rsize);
			return 0;
		}
		if(i<0) ERRR("%s: read error: %s\n", __func__, strerror(errno));
		if(i==0) INFO("%s: read, disconnected on the other end\n", __func__);
		return -1;
	}
	
	if(!flags&RAOP_FD_WRITE){
		ERRR("%s: unknow event flags=%d\n", __func__,flags);
		return -1;
	}
	
	if(!raopcld->wblk_remsize) {
		ERRR("%s: write is called with remsize=0\n", __func__);
		return -1;
	}

	INFO("Writing %d bytes to fd %d\n", raopcld->wblk_remsize, raopcld->sfd);
	i = syscalls_write(raopcld->sfd,
			   raopcld->data + raopcld->wblk_wsize,
			   raopcld->wblk_remsize);

	dump_complete_raop_play(raopcld->data + raopcld->wblk_wsize, i);

	INFO("write to %d returned %d\n", raopcld->sfd, i);

	if (i < 0) {
		ERRR("%s: write error: %s\n", __func__, strerror(errno));
		return -1;
	}
	if (i == 0) {
		INFO("%s: write, disconnected on the other end\n", __func__);
		return -1;
	}
	raopcld->wblk_wsize+=i;
	raopcld->wblk_remsize-=i;
	if(!raopcld->wblk_remsize) {
		set_fd_event(raopcld->sfd, RAOP_FD_READ, fd_event_callback,(void*)raopcld);
	}

	INFO("%d bytes are sent, remaining size=%d\n",i,raopcld->wblk_remsize);
	return 0;
}


static int raopcl_send_sample(raopcl_t *p, uint8_t *sample, int count)
{
	int rval=-1;
	uint16_t len;
        uint8_t header[] = {
		0x24, 0x00, 0x00, 0x00,
		0xF0, 0xFF, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
        };

	uint8_t **datap;

	uint8_t *encrypted_buffer;
	size_t encrypted_len;
	struct aes_data aes_data;

	const int header_size=sizeof(header);
	raopcl_data_t *raopcld;

	ERRR("Preparing to send audio sample (count: %d)\n", count);

	if (!p) return -1;

	raopcld=(raopcl_data_t *)p;

	datap = &raopcld->data;

	if (realloc_memory((void **)datap,
			   count + header_size + 16,
			   __func__)) {
		goto erexit;
	}

	memcpy(raopcld->data,header,header_size);
	len=count+header_size-4;
	ERRR("reported len: %d\n", len);

	raopcld->data[2]=len>>8;
	raopcld->data[3]=len&0xff;
	memcpy(raopcld->data+header_size,sample,count);

	memcpy(&aes_data.key, raopcld->key, RAOP_AES_KEY_LEN);
	memcpy(&aes_data.iv, raopcld->iv, RAOP_AES_IV_LEN);
	initialize_aes(&aes_data);
	encrypted_buffer = malloc(1024 * 32);
	aes_encrypt_data(&aes_data,
			 encrypted_buffer,
			 &encrypted_len,
			 raopcld->data + header_size,
			 count);
	memcpy(raopcld->data + header_size, encrypted_buffer, count);
	free(encrypted_buffer);

	len=count+header_size;
	INFO("encrypted len: %d\n", len);

	raopcld->wblk_remsize=count+header_size;
	raopcld->wblk_wsize=0;

	INFO("Setting fd event\n");

	if (set_fd_event(raopcld->sfd,
			 RAOP_FD_READ|RAOP_FD_WRITE,
			 fd_event_callback,
			 (void*)raopcld)) {

		ERRR("Failed to set fd event\n");
		goto erexit;

	}

	rval=0;

erexit:
	INFO("rval: %d\n", rval);
	return rval;
}


static int raopcl_wait_songdone(raopcl_t *p, int set)
{
	raopcl_data_t *raopcld=(raopcl_data_t *)p;

	INFO("in wait song done\n");

	if (set) {
		raopcld->wait_songdone = (set == 1);
	}

	return raopcld->wait_songdone;
}


static int raopcl_aexbuf_time(raopcl_t *p, struct timeval *dtv)
{
	raopcl_data_t *raopcld=(raopcl_data_t *)p;
	struct timeval ctv, atv;

	INFO("in aexbuf time\n");

	if (!p) return -1;

	if(raopcld->size_in_aex<=0) {
		memset(dtv, 0, sizeof(struct timeval));
		return 1; // not playing?
	}

	atv.tv_sec=raopcld->size_in_aex/44100;
	atv.tv_usec=(raopcld->size_in_aex%44100)*10000/441;

	gettimeofday(&ctv,NULL);
	dtv->tv_sec=ctv.tv_sec - raopcld->last_read_tv.tv_sec;
	dtv->tv_usec=ctv.tv_usec - raopcld->last_read_tv.tv_usec;

	dtv->tv_sec=atv.tv_sec-dtv->tv_sec;
	dtv->tv_usec=atv.tv_usec-dtv->tv_usec;
	
	if (dtv->tv_usec >= 1000000) {
		dtv->tv_sec++;
		dtv->tv_usec-=1000000;
	} else if (dtv->tv_usec < 0) {
		dtv->tv_sec--;
		dtv->tv_usec+=1000000;
	}

	if (dtv->tv_sec < 0) {
		memset(dtv, 0, sizeof(struct timeval));
	}

	INFO("%s:tv_sec=%d, tv_usec=%d\n",__func__,(int)dtv->tv_sec,(int)dtv->tv_usec);

	return 0;
}


static int main_event_handler()
{
	fd_set rdfds,wrfds;
	int fdmax=0;
	int i;
	struct timeval tout={.tv_sec=MAIN_EVENT_TIMEOUT, .tv_usec=0};

	INFO("in main event handler\n");

	FD_ZERO(&rdfds);
	FD_ZERO(&wrfds);

	for (i=0 ; i < MAX_NUM_OF_FDS ; i++) {
		if (raopld->fds[i].fd < 0) {
			continue;
		}

		if (raopld->fds[i].flags & RAOP_FD_READ) {
			INFO("Adding fd %d to read set\n", raopld->fds[i].fd);
			FD_SET(raopld->fds[i].fd, &rdfds);
		}

		if (raopld->fds[i].flags & RAOP_FD_WRITE) {
			INFO("Adding fd %d to write set\n", raopld->fds[i].fd);
			FD_SET(raopld->fds[i].fd, &wrfds);
		}

		fdmax = (fdmax < raopld->fds[i].fd) ? raopld->fds[i].fd : fdmax;
	}

	if (raopcl_wait_songdone(raopld->raopcl,0)) {
		raopcl_aexbuf_time(raopld->raopcl, &tout);
	}

	select(fdmax + 1, &rdfds, &wrfds, NULL, &tout);

	for(i=0;i<MAX_NUM_OF_FDS;i++){
		if (raopld->fds[i].fd < 0) {
			continue;
		}

		if ((raopld->fds[i].flags & RAOP_FD_READ) &&
		    FD_ISSET(raopld->fds[i].fd, &rdfds)) {
			INFO("rd event i=%d, flags=%d\n",i,raopld->fds[i].flags);
			if (raopld->fds[i].cbf &&
			    raopld->fds[i].cbf(raopld->fds[i].dp, RAOP_FD_READ)) {
				return -1;
			}
		}

		if((raopld->fds[i].flags&RAOP_FD_WRITE) &&
		   FD_ISSET(raopld->fds[i].fd,&wrfds)){
			INFO("wr event i=%d, flags=%d\n",i,raopld->fds[i].flags);
			if(raopld->fds[i].cbf &&
			   raopld->fds[i].cbf(raopld->fds[i].dp, RAOP_FD_WRITE)) return -1;
		}
	}

	if(raopcl_wait_songdone(raopld->raopcl,0)){
		raopcl_aexbuf_time(raopld->raopcl, &tout);
		if(!tout.tv_sec && !tout.tv_usec){
			// AEX data buffer becomes empty, it means end of playing a song.
			INFO("%s\n",RAOP_SONGDONE);
			fflush(stdout);
			raopcl_wait_songdone(raopld->raopcl,-1); // clear wait_songdone
		}
	}

	return 0;
}


static raopcl_t *raopcl_open(struct aes_data *aes_data)
{
	raopcl_data_t *raopcld;

	WARN("Starting client open\n");

	raopcld=syscalls_malloc(sizeof(raopcl_data_t));
	memset(raopcld, 0, sizeof(raopcl_data_t));

	INFO("sizeof(raopcld->iv): %d sizeof(aes_data->iv): %d\n",
	     sizeof(raopcld->iv), sizeof(aes_data->iv));

	if (sizeof(raopcld->iv) != sizeof(aes_data->iv)) {
		CRIT("FATAL: IV sizes do not match\n");
		exit (0);
	}
	syscalls_memcpy(raopcld->iv, aes_data->iv, sizeof(raopcld->iv));

	INFO("sizeof(raopcld->key): %d sizeof(aes_data->key): %d\n",
	     sizeof(raopcld->key), sizeof(aes_data->key));

	if (sizeof(raopcld->key) != sizeof(aes_data->key)) {
		CRIT("FATAL: AES key sizes do not match\n");
		exit (0);
	}
	syscalls_memcpy(raopcld->key, aes_data->key, sizeof(raopcld->key));

	memcpy(raopcld->nv,raopcld->iv,sizeof(raopcld->nv));
	raopcld->volume = VOLUME_DEF;

	return (raopcl_t *)raopcld;
}


static int raopcl_sample_remsize(raopcl_t *p)
{
	raopcl_data_t *raopcld=(raopcl_data_t *)p;
	if(!p) return -1;
	return raopcld->wblk_remsize;
}


int hacked_send_audio(char *pcm_audio_file, int session_fd, struct aes_data *aes_data)
{
	int rval = 0;
	uint8_t *buf = NULL;
	int size = 0;
	raopcl_data_t *raopcl;
	int i;
	int pcm_datafile_open = 0;

	INFO("PCM audio file: \"%s\" session_fd: %d\n", pcm_audio_file, session_fd);

	raopld = syscalls_malloc(sizeof(*raopld));
	for (i = 0 ; i < MAX_NUM_OF_FDS ; i++) {
		raopld->fds[i].fd = -1;
	}
	raopld->auds = auds_open(pcm_audio_file, AUD_TYPE_PCM);
	pcm_datafile_open = 1;
	raopld->raopcl = raopcl_open(aes_data);

	raopcl = (raopcl_data_t *)raopld->raopcl;
	raopcl->sfd = session_fd;

	rval=0;
	while(!rval){

		INFO("rval: %d\n", rval);

		if(!raopld->auds){
			WARN("audio data closed\n");
			// if audio data is not opened, just check events
			rval=main_event_handler(raopld);
			continue;
		}

		if(pcm_get_next_sample(raopld->auds, &buf, &size)){
			auds_close(raopld->auds);
			raopld->auds=NULL;
			raopcl_wait_songdone(raopld->raopcl,1);
		}
		if(raopcl_send_sample(raopld->raopcl,buf,size)) break;
		do{
			if((rval=main_event_handler())) break;
		}while(raopld->auds && raopcl_sample_remsize(raopld->raopcl));
	}

	return 0;
}


void test_audio(void)
{
	struct aes_data aes_data;
	char pcm_testdata[] = "pcm_testfile";
	int session_fd;

	CRIT("Testing audio send routines; no connections "
	     "will be made to the server\n");

	generate_aes_data(&aes_data);

	session_fd = syscalls_open("./fake_session",
				   O_RDWR, S_IRUSR | S_IWUSR);

	hacked_send_audio(pcm_testdata,
			  session_fd,
			  &aes_data);

	CRIT("Audio test done; exiting\n");

	exit (1);
}
