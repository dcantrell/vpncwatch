#
# Makefile for vpncwatch
# David Cantrell <dcantrell@redhat.com>
#

DIST_FILES = AUTHORS COPYING README Makefile vpncwatch.c vpncwatch.h \
             vpnc-watch.py proc.c net.c

SRCS = vpncwatch.c proc.c net.c
OBJS = vpncwatch.o proc.o net.o

CC     ?= gcc
CFLAGS = -D_GNU_SOURCE -O2 -Wall -Werror

REPO = https://vpncwatch.googlecode.com/svn

vpncwatch: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

ChangeLog:
	svn log -v > ChangeLog

install:
	@echo "No."

tag: vpncwatch
	TAG=`./vpncwatch -V 2>/dev/null` ; \
	svn copy $(REPO)/trunk $(REPO)/tags/$$TAG

dist-gzip: tag
	TAG=`./vpncwatch -V 2>/dev/null` ; \
	rm -rf $$TAG ; \
	svn co $(REPO)/tags/$$TAG $$TAG ; \
	cd $$TAG ; \
	svn log -v > ChangeLog ; \
	rm -rf .svn ; \
	cd .. ; \
	tar -cf - $$TAG | gzip -9c > $$TAG.tar.gz ; \
	rm -rf $$TAG

clean:
	-rm -rf $(OBJS) vpncwatch ChangeLog vpncwatch-$(VER)*
