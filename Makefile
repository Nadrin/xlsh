# xlsh - eXtended Login Shell
# See COPYING file for license details.

.DEFAULT_GOAL = all
.PHONY: all install install-strip installdirs uninstall clean

prefix = /usr/local
exec_prefix = $(prefix)
bin_dir = $(exec_prefix)/bin
sbindir = $(exec_prefix)/sbin
datarootdir = $(prefix)/share
datadir = $(datarootdir)
sysconfdir = $(prefix)/etc
sharedstatedir = $(prefix)/com
localstatedir = $(prefix)/var

CFLAGS += -I./include -g -Wall

vpath %.c ./src

PROGRAMS      = xlsh xlshd

XLSH_OBJ      = xlsh.o libxlsh.o
XLSH_SOURCE   = xlsh.c libxlsh.c
XLSH_HEADERS  = xlsh.h libxlsh.h config.h
XLSH_LIBS     = -lreadline -lpam

XLSHD_OBJ     = xlshd.o libxlsh.o
XLSHD_SOURCE  = xlshd.c libxlsh.c
XLSHD_HEADERS = config.h libxlsh.h

all: $(PROGRAMS)

xlsh: $(XLSH_OBJ) $(XLSH_LIBS)

xlshd: $(XLSHD_OBJ)

install: installdirs
	install -m 755 xlsh $(DESTDIR)$(sbindir)
	install -m 755 xlshd $(DESTDIR)$(sbindir)
	install -d -m 755 $(DESTDIR)$(sysconfdir)/xlsh
	install -m 644 etc/xlshrc $(DESTDIR)$(sysconfdir)/xlsh
	install -m 644 etc/Xresources $(DESTDIR)$(sysconfdir)/xlsh

install-strip: installdirs
	install -s -m 755 xlsh $(DESTDIR)$(sbindir)
	install -s -m 755 xlshd $(DESTDIR)$(sbindir)
	install -m 644 etc/xlshrc $(DESTDIR)$(sysconfdir)/xlsh
	install -m 644 etc/Xresources $(DESTDIR)$(sysconfdir)/xlsh

installdirs:
	install -d $(DESTDIR)$(sbindir) $(DESTDIR)$(sysconfdir)/xlsh

uninstall:
	rm -f ${DESTDIR}$(sbindir)/xlsh
	rm -f ${DESTDIR}$(sbindir)/xlshd

clean:
	rm -f $(PROGRAMS)
	rm -f $(XLSH_OBJ) $(XLSHD_OBJ)

