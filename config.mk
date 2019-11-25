VERSION = 0.1.0

PREFIX = /usr
MANPREFIX = ${PREFIX}/share/man

ELOGIND = 0
ifneq "$(ELOGIND)" "0"
    SDBUSLIB:=libelogind
else
    SDBUSLIB:=libsystemd
endif

INCS = -I. -I/usr/include
LIBS = -L/usr/lib -lc

CPPFLAGS = -DVERSION=\"${VERSION}\" -DELOGIND="${ELOGIND}" -D_GNU_SOURCE
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Os -s ${INCS} ${CPPFLAGS} $(shell pkg-config --cflags ${SDBUSLIB} libbsd)
LDFLAGS = ${LIBS} $(shell pkg-config --libs ${SDBUSLIB} libbsd)

CC = cc
LD = ld
