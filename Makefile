CC = gcc
CFLAGS = -Wall -O3 -D_GNU_SOURCE
LDFLAGS=
OBJECTS = libGIS-1.0.5/atmel_generic.o libGIS-1.0.5/ihex.o libGIS-1.0.5/srecord.o avr_instruction_set.o opcode_stream.o disasm_stream.o format_stream.o main.o
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

