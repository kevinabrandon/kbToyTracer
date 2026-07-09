# kbSPD -- Eric Haines's Standard Procedural Databases, emitting kbToyTracer
# .sdf scenes (output format 20, the default).  See README.md for provenance.

CC      ?= cc
CFLAGS  ?= -O2
LDLIBS   = -lm

GENERATORS = balls gears mount rings teapot tetra tree \
             jacks lattice sample shells sombrero nurbtst
READERS    = readdxf readnff readobj

LIBOBJ = drv_null.o libini.o libinf.o libpr1.o libpr2.o libpr3.o \
         libply.o libdmp.o libvec.o libtx.o

all: $(GENERATORS) $(READERS) nff2rad

$(LIBOBJ): def.h lib.h libvec.h drv.h

$(GENERATORS) $(READERS): %: %.c $(LIBOBJ)
	$(CC) $(CFLAGS) -o $@ $< $(LIBOBJ) $(LDLIBS)

nff2rad: nff2rad.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(GENERATORS) $(READERS) nff2rad $(LIBOBJ)

.PHONY: all clean
