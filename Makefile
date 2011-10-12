# xlsh - eXtended Login Shell
# See COPYING file for license details.

DESTDIR ?= /usr/local
CONFDIR ?= /etc

XLSH_SOURCE   = src/xlsh.c src/libxlsh.c
XLSH_HEADERS  = include/xlsh.h include/libxlsh.h include/config.h
XLSH_LIBS     = -lreadline -lpam

XLSHD_SOURCE  = src/xlshd.c src/libxlsh.c
XLSHD_HEADERS = include/config.h include/libxlsh.h

all: xlsh xlshd

xlsh: ${XLSH_SOURCE} ${XLSH_HEADERS}
	${CC} -g -Wall -o $@ -I./include ${XLSH_SOURCE} ${CFLAGS} ${LDFLAGS} ${XLSH_LIBS}

xlshd: ${XLSHD_SOURCE} ${XLSHD_HEADERS}
	${CC} -g -Wall -o $@ -I./include ${XLSHD_SOURCE} ${CFLAGS} ${LDFLAGS}

install: all
	install -m 755 xlsh ${DESTDIR}/sbin/
	install -m 755 xlshd ${DESTDIR}/sbin/
	install -d -m 755 ${CONFDIR}/xlsh
	install -m 644 etc/xlshrc ${CONFDIR}/xlsh
	install -m 644 etc/Xresources ${CONFDIR}/xlsh

clean:
	rm -f xlsh xlshd

.PHONY: all install clean
