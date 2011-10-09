# xlsh - eXtended Login Shell
# See COPYING file for license details.

XLSH_SOURCE  = src/xlsh.c src/libxlsh.c
XLSH_HEADERS = src/xlsh.h src/libxlsh.h src/config.h
XLSH_LIBS    = -lreadline -lpam

XLSHD_SOURCE = src/xlshd.c src/libxlsh.c
XLSHD_HEADERS= src/config.h src/libxlsh.h

all: xlsh xlshd

xlsh: ${XLSH_SOURCE} ${XLSH_HEADERS}
	${CC} -g -Wall -o $@ ${XLSH_SOURCE} ${CFLAGS} ${LDFLAGS} ${XLSH_LIBS}

xlshd: ${XLSHD_SOURCE} ${XLSHD_HEADERS}
	${CC} -g -Wall -o $@ ${XLSHD_SOURCE} ${CFLAGS} ${LDFLAGS}

clean:
	rm -f xlsh xlshd

.PHONY: all clean
