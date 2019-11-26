.POSIX:

NAME = caffeinated

VERSION != grep VERSION version.h | cut -d \" -f2

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

SDBUS = systemd
SDBUS.lib.systemd = -lsystemd
SDBUS.lib.eligind = -lelogind
SDBUS.lib = $(SDBUS.lib.$(SDBUS))
SDBUS.define.elogind = -DELOGIND
SDBUS.define = $(SDBUS.define.$(SDBUS))

CC = cc
LD = ld
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Os -s -D_GNU_SOURCE $(SDBUS.define)
LDLIBS = -lbsd $(SDBUS.lib)

SRC = main.c
OBJ = $(SRC:.c=.o)

all: options $(NAME)

options:
	@echo $(NAME) build options:
	@echo "SDBUS    = $(SDBUS)"
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "LDLIBS   = $(LDLIBS)"

$(NAME): $(OBJ)
	$(CC) $(LDFLAGS) -o $(NAME) $(OBJ) $(LDLIBS)

$(OBJ): $(SRC) version.h

clean:
	@echo cleaning
	rm -f $(NAME) $(OBJ)

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
	rm -f $(DESTDIR)$(PREFIX)/bin/$(NAME)
	rm -f $(DESTDIR)$(MANPREFIX)/man1/$(NAME).1
