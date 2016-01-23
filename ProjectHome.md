raopd was intended to be software to stream audio from a Linux box to an Apple AirPort Express.  After getting an initial proof-of-concept working, I concluded that I have better uses of my time.  pulseaudio has also integrated support for streaming to AEX, although I have not used it.



What follows is my original project description:

The system consists of two pieces.  The first piece is an ALSA kernel driver to provide an audio device for applications to use for audio output.  The second piece is a simple Linux daemon that packages up the stream, encrypts it, and transmits it over the network using the RAOP protocol.

raopd differs from raop\_play and JustePort, two other RAOP implementations, in that raopd is not a music player, nor does it have a user interface--it is in many ways a simplified version of raop\_play.  It is a kernel module and system daemon.  Audio applications see it as a hardware sound card, and it transmits the audio stream it receives unmodified to the AEX.

One other significant change, not technical, is that raopd is a pure GPLv3 implementation.  All the code has been written by us, and it depends only on OpenSSL, with an appropriate license exception under the GPLv3 to permit linking against OpenSSL.

One other goal of this project is to create a software package that "just works".  The module will load and the daemon will be started on boot via an init script, remaining idle until audio data is present, and returning to idle and releasing control of the AirPort when the audio stream ends.

The code is net yet fully functional.  If you've arrived here looking for working software, please check back in a month or so.  While the core code works, and we're able to stream audio to the AirPort, much of the supporting code still remains to be written, and installation is a purely manual process.  Unless you're a C programmer, there isn't going to be anything useful or interesting here (and even then, it's probably not that interesting!)  If you want something that works now, try http://raop-play.sourceforge.net/ or http://nanocr.eu/software/justeport/