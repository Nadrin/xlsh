# xlsh - eXtended Login Shell
# See COPYING file for license details.

.DEFAULT_GOAL = all
.PHONY: all install install-strip installdirs uninstall clean

prefix = /usr/local

exec_prefix = $(prefix)
bindir      = $(exec_prefix)/bin
sbindir     = $(exec_prefix)/sbin
sysconfdir  = /etc

SHELL = /bin/sh
INSTALL = install
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

xlsh: $(XLSH_OBJ)
xlsh: LDLIBS=$(XLSH_LIBS)

xlshd: $(XLSHD_OBJ)

install: installdirs
	$(INSTALL_PROGRAM) xlsh $(DESTDIR)$(sbindir)
	$(INSTALL_PROGRAM) xlshd $(DESTDIR)$(sbindir)
	$(INSTALL_DATA) etc/xlshrc $(DESTDIR)$(sysconfdir)/xlsh
	$(INSTALL_DATA) etc/Xresources $(DESTDIR)$(sysconfdir)/xlsh
	$(INSTALL_DATA) etc/pam.d/xlshd $(DESTDIR)$(sysconfdir)/pam.d

install-strip: installdirs
	$(INSTALL_PROGRAM_STRIP) xlsh $(DESTDIR)$(sbindir)
	$(INSTALL_PROGRAM_STRIP) xlshd $(DESTDIR)$(sbindir)
	$(INSTALL_DATA) etc/xlshrc $(DESTDIR)$(sysconfdir)/xlsh
	$(INSTALL_DATA) etc/Xresources $(DESTDIR)$(sysconfdir)/xlsh

installdirs:
	$(INSTALL_DIR) $(DESTDIR)$(sbindir)
	$(INSTALL_DIR) $(DESTDIR)$(sysconfdir)/xlsh
	$(INSTALL_DIR) $(DESTDIR)$(sysconfdir)/pam.d

uninstall:
	rm -f ${DESTDIR}$(sbindir)/xlsh
	rm -f ${DESTDIR}$(sbindir)/xlshd

clean:
	rm -f $(PROGRAMS)
	rm -f $(XLSH_OBJ) $(XLSHD_OBJ)

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(ALL_CFLAGS) $< -o $@
