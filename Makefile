CC = gcc
CFLAGS = -Wall -O3 -D_GNU_SOURCE
LDFLAGS=
OBJECTS = libGIS-1.0.3/atmel_generic.o libGIS-1.0.3/ihex.o libGIS-1.0.3/srecord.o avr_instructionset.o avr_disasm.o format.o file.o ui.o
PROGNAME = vavrdisasm

all: $(PROGNAME)

$(PROGNAME): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) 

clean:
	rm -rf $(PROGNAME) libGIS-1.0.3/*.o *.o
