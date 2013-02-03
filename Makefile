CC = gcc
CFLAGS = -Wall -O3 -D_GNU_SOURCE -I.
LDFLAGS=
LIBGIS_OBJECTS = libGIS-1.0.5/atmel_generic.o libGIS-1.0.5/ihex.o libGIS-1.0.5/srecord.o
CORE_OBJECTS = byte_stream.o main.o
AVR_OBJECTS = avr/avr_instruction_set.o avr/avr_disasm_stream.o avr/avr_print_stream.o
OBJECTS = $(LIBGIS_OBJECTS) $(CORE_OBJECTS) $(AVR_OBJECTS)
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

