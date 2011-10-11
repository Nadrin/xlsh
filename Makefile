# xlsh - eXtended Login Shell
# See COPYING file for license details.

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

clean:
	rm -f xlsh xlshd

.PHONY: all clean
