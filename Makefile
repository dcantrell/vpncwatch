#
# Makefile for vpncwatch
# David Cantrell <dcantrell@redhat.com>
#

SRCS = vpncwatch.c proc.c net.c
HDRS = vpncwatch.h
OBJS = vpncwatch.o proc.o net.o

DISTFILES = AUTHORS COPYING README Makefile $(SRCS) $(HDRS) vpnc-watch.py

CC     ?= gcc
CFLAGS = -D_GNU_SOURCE -O2 -Wall -Werror

# Update version in vpncwatch.h as well
TAG    = vpncwatch-1.8

vpncwatch: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

ChangeLog:
	git log > ChangeLog

install:
	@echo "No."

tag:
	@git tag -s -a -m "Tag as $(TAG)" $(TAG)
	@echo "Tagged as $(TAG)"

release: ChangeLog
	rm -rf $(TAG)
	mkdir -p $(TAG)
	cp -pr $(DISTFILES) ChangeLog openwrt $(TAG)
	tar -cf - $(TAG) | gzip -9c > $(TAG).tar.gz
	rm -rf $(TAG)

clean:
	-rm -rf $(OBJS) vpncwatch ChangeLog $(TAG)*
