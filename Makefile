
WITHAUDIO=yes
# WITHAUDIO=no

PREFIX=.
DATADIR=${PREFIX}/share/subharmonic-traumaphone

LIBS=-lm

MYCFLAGS=-DPREFIX=${PREFIX} ${DEBUGFLAG} ${PROFILEFLAG} ${OPTIMIZEFLAG} \
	--pedantic -Wall ${STOP_ON_WARN} -pthread -std=gnu99 -rdynamic

ifeq (${WITHAUDIO},yes)
SNDLIBS:=$(shell pkg-config --libs portaudio-2.0 vorbisfile)
SNDFLAGS:=-DWITHAUDIOSUPPORT $(shell pkg-config --cflags portaudio-2.0) -DDATADIR=\"${DATADIR}\"
OGGOBJ=ogg_to_pcm.o
SNDOBJS=wwviaudio.o
else
SNDLIBS=
SNDFLAGS=-DWWVIAUDIO_STUBS_ONLY
OGGOBJ=
SNDOBJS=wwviaudio.o
endif

GTKCFLAGS:=$(subst -I,-isystem ,$(shell pkg-config --cflags gtk+-2.0))
GTKLDFLAGS:=$(shell pkg-config --libs gtk+-2.0) $(shell pkg-config --libs gthread-2.0)

VORBISFLAGS:=$(subst -I,-isystem ,$(shell pkg-config --cflags vorbisfile))
SNDFLAGS:=-DWITHAUDIOSUPPORT $(shell pkg-config --cflags portaudio-2.0) -DDATADIR=\"${DATADIR}\"
OBJS=wwviaudio.o ogg_to_pcm.o

VORBISCOMPILE=$(CC) ${MYCFLAGS} ${GTKCFLAGS} ${VORBISFLAGS} ${SNDFLAGS} -c -o $@ $< && $(ECHO) '  COMPILE' $<

VORBISLINK=$(CC) ${MYCFLAGS} ${SNDFLAGS} ${GTKCFLAGS} -o $@ ${GTKCFLAGS} ${GTKLDFLAGS} ${LIBS} ${SNDLIBS} && $(ECHO) '  LINK' $@


ifeq (${E},1)
STOP_ON_WARN=-Werror
else
STOP_ON_WARN=
endif

ifeq (${O},1)
DEBUGFLAG=
OPTIMIZEFLAG=-O3
else
DEBUGFLAG=-g
OPTIMIZEFLAG=
endif

ifeq (${P},1)
PROFILEFLAG=-pg
OPTIMIZEFLAG=-O3
DEBUGFLAG=
else
PROFILEFLAG=
endif

ifeq (${V},1)
Q=
ECHO=true
else
Q=@
ECHO=echo
endif

all:	subharmonic-traumaphone

wwviaudio.o:    wwviaudio.c Makefile
	$(Q)$(VORBISCOMPILE)

subharmonic-traumaphone.o:	subharmonic-traumaphone.c wwviaudio.o ogg_to_pcm.o
	$(Q)$(VORBISCOMPILE)

subharmonic-traumaphone:	subharmonic-traumaphone.o
	gcc ${OPTIMIZEFLAG} ${GTKCFLAGS} -o subharmonic-traumaphone -lm subharmonic-traumaphone.o wwviaudio.o ogg_to_pcm.o ${SNDLIBS} ${GTKLDFLAGS}


	


