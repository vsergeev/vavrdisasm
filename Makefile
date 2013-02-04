CC = gcc
CFLAGS = -Wall -O3 -D_GNU_SOURCE -I.
LDFLAGS=
LIBGIS_OBJECTS = file/libGIS-1.0.5/atmel_generic.o file/libGIS-1.0.5/ihex.o file/libGIS-1.0.5/srecord.o
FILE_OBJECTS = $(LIBGIS_OBJECTS) file/atmel_generic.o file/ihex.o file/srecord.o file/binary.o file/debug.o file/test.o file/asciihex.o
AVR_OBJECTS = avr/avr_instruction_set.o avr/avr_disasm.o avr/avr_print.o
PRINT_OBJECTS = print_stream.o
OBJECTS = $(FILE_OBJECTS) $(AVR_OBJECTS) $(PRINT_OBJECTS) main.o

PROGNAME = vavrdisasm
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: $(PROGNAME)

install: $(PROGNAME)
	install -D -s -m 0755 $(PROGNAME) $(DESTDIR)$(BINDIR)/$(PROGNAME)

$(PROGNAME): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

clean:
	rm -rf $(PROGNAME) $(OBJECTS)

test: $(PROGNAME)
	python2 crazy_test.py

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(PROGNAME)

