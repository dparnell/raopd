TARGETS := raopd

RAOPD_OBJS += main.o
RAOPD_OBJS += lt.o
RAOPD_OBJS += utility.o
RAOPD_OBJS += syscalls.o
RAOPD_OBJS += encryption.o
RAOPD_OBJS += encoding.o
RAOPD_OBJS += client.o
RAOPD_OBJS += config.o
RAOPD_OBJS += rtsp.o
RAOPD_OBJS += rtsp_client.o
RAOPD_OBJS += sdp.o
RAOPD_OBJS += audio_stream.o
RAOPD_OBJS += raop_play_send_audio.o
RAOPD_OBJS += aes.o

RAOPD_HEADERS += *.h

CC := gcc

# Should enable -Wconversion with gcc4.3
CFLAGS += -ggdb -Wall -Wextra -Wunused -Werror -Wshadow \
	-Wcast-qual -Wcast-align -Wwrite-strings -Wswitch-default \
	-Wdeclaration-after-statement -Wmissing-prototypes \
	-pedantic-errors -O2 -std=c99 -pthread -D_GNU_SOURCE -D_REENTRANT

LINK_FLAGS += -lpthread -lcrypto

.PHONY: all
all: $(TARGETS)

raopd: 		$(RAOPD_OBJS)
		$(CC) $(LINK_FLAGS) -o raopd $(RAOPD_OBJS)

# Just rebuild everything if any of the headers change.  It doesn't
# take very long, and it's easier than trying to track the header
# dependencies here.  If anybody knows a cleaner way to do this, I'd
# love to hear about it.
$(RAOPD_OBJS): $(RAOPD_HEADERS) Makefile

.PHONY: clean
clean: 
		rm -f $(TARGETS) $(RAOPD_OBJS)
