PROGNAME = vavrdisasm
PREFIX ?= /usr
BINDIR = $(PREFIX)/bin

################################################################################

LIBGIS_SOURCES = file/libGIS-1.0.5/atmel_generic.c file/libGIS-1.0.5/ihex.c file/libGIS-1.0.5/srecord.c
FILE_SOURCES = file/atmel_generic.c file/ihex.c file/srecord.c file/binary.c file/debug.c file/test.c file/asciihex.c
AVR_SOURCES = avr/avr_instruction_set.c avr/avr_disasm.c avr/avr_print.c
PRINT_SOURCES = print_stream.c
SOURCES = $(LIBGIS_SOURCES) $(FILE_SOURCES) $(AVR_SOURCES) $(PRINT_SOURCES) main.c

################################################################################

BUILD_DIR = build
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

################################################################################

CC = gcc
CFLAGS = -Wall -O3 -D_GNU_SOURCE -I.
LDFLAGS=

################################################################################

all: $(PROGNAME)

$(PROGNAME): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	rm -rf $(PROGNAME) $(BUILD_DIR)

test: $(PROGNAME)
	python2 crazy_test.py

install: $(PROGNAME)
	mkdir -p $(DESTDIR)$(BINDIR)
	install -s -m 0755 $(PROGNAME) $(DESTDIR)$(BINDIR)/$(PROGNAME)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(PROGNAME)

################################################################################

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

