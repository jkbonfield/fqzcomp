VERS=4.6

all: fqzcomp

CC = g++
#CC=~rmd/opt/llvm-3.2/bin/clang++
#CC=icpc

#CFLAGS  = -g -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
CFLAGS  = -O3 -g -fomit-frame-pointer -fstrict-aliasing -ffast-math -DNDEBUG -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -Wall -msse2

#CC=icpc
#CFLAGS  = -O3 -g -fomit-frame-pointer -fstrict-aliasing -fast -DNDEBUG -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -Wall -m64 -msse2

CFLAGS += -DPTHREADS
LIBS += -lpthread

#CFLAGS += -DORIG_MODEL

.c.o:
	$(CC) $(CFLAGS) -c $< 

fqzcomp: fqzcomp.o sfh.o
	$(CC) $(CFLAGS) -o $@ fqzcomp.o sfh.o $(LIBS)

clean:
	-rm *.o fqzcomp

dist:
	-rm -rf fqzcomp-$(VERS)
	mkdir fqzcomp-$(VERS)
	cp *.[ch] *.cdr fqzcomp-$(VERS)
	cp README Makefile fqzcomp-$(VERS)
	tar cvfz fqzcomp-$(VERS).tar.gz fqzcomp-$(VERS)
