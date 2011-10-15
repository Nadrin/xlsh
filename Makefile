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

SHELL = /bin/sh
INSTALL = /usr/bin/install
INSTALL_DIR = $(INSTALL) -d
INSTALL_DATA = $(INSTALL) -m 644
INSTALL_PROGRAM = $(INSTALL)
INSTALL_PROGRAM_STRIP = $(INSTALL) -s

CFLAGS += -g -Wall
ALL_CFLAGS = -I./include $(CFLAGS)

.SUFFIXES:
.SUFFIXES: .o

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
	$(INSTALL_PROGRAM) xlsh $(DESTDIR)$(sbindir)
	$(INSTALL_PROGRAM) xlshd $(DESTDIR)$(sbindir)
	$(INSTALL_DATA) etc/xlshrc $(DESTDIR)$(sysconfdir)/xlsh
	$(INSTALL_DATA) etc/Xresources $(DESTDIR)$(sysconfdir)/xlsh

install-strip: installdirs
	$(INSTALL_PROGRAM_STRIP) xlsh $(DESTDIR)$(sbindir)
	$(INSTALL_PROGRAM_STRIP) xlshd $(DESTDIR)$(sbindir)
	$(INSTALL_DATA) etc/xlshrc $(DESTDIR)$(sysconfdir)/xlsh
	$(INSTALL_DATA) etc/Xresources $(DESTDIR)$(sysconfdir)/xlsh

installdirs:
	$(INSTALL_DIR) $(DESTDIR)$(sbindir) $(DESTDIR)$(sysconfdir)/xlsh

uninstall:
	rm -f ${DESTDIR}$(sbindir)/xlsh
	rm -f ${DESTDIR}$(sbindir)/xlshd

clean:
	rm -f $(PROGRAMS)
	rm -f $(XLSH_OBJ) $(XLSHD_OBJ)

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(ALL_CFLAGS) $< -o $@
