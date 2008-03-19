#
# Makefile for vpncwatch
# David Cantrell <dcantrell@redhat.com>
#

DIST_FILES = AUTHORS COPYING README Makefile vpncwatch.c vpncwatch.h \
             vpnc-watch.py ChangeLog proc.c net.c

SRCS = vpncwatch.c proc.c net.c
OBJS = vpncwatch.o proc.o net.o

CC     ?= gcc
CFLAGS = -D_GNU_SOURCE -O2 -Wall -Werror

vpncwatch: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

ChangeLog:
	( GIT_DIR=.git git-log > .changelog.tmp && mv .changelog.tmp ChangeLog ; rm -f .changelog.tmp ) || ( touch ChangeLog ; echo 'git directory not found: installing possibly empty ChangeLog.' >&2)

install:
	@echo "No."

dist-gzip: ChangeLog vpncwatch
	PKG=`./vpncwatch -V 2>/dev/null` ; \
	rm -rf $$PKG ; \
	mkdir -p $$PKG ; \
	cp -p $(DIST_FILES) $$PKG ; \
	tar -cf - $$PKG | gzip -9c > $$PKG.tar.gz ; \
	rm -rf $$PKG

clean:
	-rm -rf $(OBJS) vpncwatch ChangeLog vpncwatch-$(VER)*
