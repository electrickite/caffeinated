.POSIX:

NAME = caffeinated

VERSION = $(shell grep VERSION version.h | cut -d \" -f2)

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

SDBUS_LIB = $(shell pkgconf --libs libsystemd)

ifeq ($(SDBUS), elogind)
	SDBUS_LIB = -lelogind
else ifeq ($(SDBUS), systemd)
	SDBUS_LIB = -lsystemd
else ifdef SDBUS
    $(error Invalid SDBUS)
endif

ifeq ($(strip $(SDBUS_LIB)), -lelogind)
	SDBUS_DEFINE = -DELOGIND
	SDBUS = elogind
endif

ifdef WAYLAND
	WAYLAND_LIB = $(shell pkg-config --silence-errors --libs wayland-client)
	WAYLAND_PROTOCOLS_DIR = $(shell pkg-config --silence-errors --variable=pkgdatadir wayland-protocols)
	WAYLAND_SCANNER = $(shell pkg-config --silence-errors --variable=wayland_scanner wayland-scanner)

	WAYLAND_PROTOCOL = $(WAYLAND_PROTOCOLS_DIR)/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml
	WAYLAND_SRC = idle-inhibit-unstable-v1-client-protocol.h idle-inhibit-unstable-v1-protocol.c
	WAYLAND_DEFINE = -DWAYLAND
endif

CC = cc
LD = ld
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Werror -Wno-unused-parameter -Os -s -D_GNU_SOURCE $(SDBUS_DEFINE) $(WAYLAND_DEFINE)
LDLIBS = -lbsd $(SDBUS_LIB) $(WAYLAND_LIB)

SRC = main.c version.h $(WAYLAND_SRC)
OBJ = main.o

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(LDLIBS) -o $(NAME) $(OBJ) $(WAYLAND_SRC) $(LDFLAGS)

$(OBJ): $(SRC)

idle-inhibit-unstable-v1-client-protocol.h:
	$(WAYLAND_SCANNER) client-header $(WAYLAND_PROTOCOL) idle-inhibit-unstable-v1-client-protocol.h

idle-inhibit-unstable-v1-protocol.c:
	$(WAYLAND_SCANNER) private-code $(WAYLAND_PROTOCOL) idle-inhibit-unstable-v1-protocol.c

clean:
	@echo cleaning
	$(RM) $(NAME) $(OBJ) $(WAYLAND_SRC)

install: all
	@echo installing in $(DESTDIR)$(PREFIX)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(NAME) $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(NAME)
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < $(NAME).1 > $(DESTDIR)$(MANPREFIX)/man1/$(NAME).1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(NAME).1

uninstall:
	@echo removing files from $(DESTDIR)$(PREFIX)
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(NAME)
	$(RM) $(DESTDIR)$(MANPREFIX)/man1/$(NAME).1
