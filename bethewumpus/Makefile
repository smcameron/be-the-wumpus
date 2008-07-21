

SNDLIBS=-lvorbisfile -lportaudio

DEBUG=-g
# DEBUG=
#PROFILE_FLAG=-pg
#PROFILE_FLAG=
#OPTIMIZE_FLAG=
# OPTIMIZE_FLAG=-O2

LDFLAGS=${PROFILE_FLAG}

all:	bethewumpus

rumble.o:	rumble.c rumble.h Makefile
	gcc ${DEBUG} -c rumble.c

joystick.o:	joystick.c joystick.h Makefile
	gcc ${DEBUG} -c joystick.c

ogg_to_pcm.o:	ogg_to_pcm.c ogg_to_pcm.h Makefile
	gcc ${DEBUG} ${PROFILE_FLAG} ${OPTIMIZE_FLAG} `pkg-config --cflags vorbisfile` \
		-pthread -Wall -c ogg_to_pcm.c

bethewumpus:	bethewumpus.c joystick.o ogg_to_pcm.o rumble.o Makefile
	gcc ${DEBUG} ${PROFILE_FLAG} ${OPTIMIZE_FLAG} -Wall joystick.o ogg_to_pcm.o rumble.o \
		bethewumpus.c -o bethewumpus -lm ${SNDLIBS} \
		`pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0` `pkg-config --libs gthread-2.0`

tarball:
	mkdir -p d/bethewumpus-${VERSION}/sounds
	cp Makefile bethewumpus.c ogg_to_pcm.c ogg_to_pcm.h \
		bethewumpus.6 joystick.h joystick.c README AUTHORS \
		COPYING TODO d/bethewumpus-${VERSION}
	cp sounds/*.ogg d/bethewumpus-${VERSION}/sounds
	cp sounds/Attribution.txt d/bethewumpus-${VERSION}/sounds
	cp sounds/Makefile d/bethewumpus-${VERSION}/sounds
	chown -R root:root d;
	( cd d; tar cvf ../bethewumpus-${VERSION}.tar ./bethewumpus-${VERSION} )
	gzip bethewumpus-${VERSION}.tar

clean:
	rm -f ./bethewumpus ./bethewumpus-*.tar.gz
	rm -fr ./d
