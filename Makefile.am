#
# Makefile.am
#
# Copyright (c) 2009-2010 EPIPE Communications. All rights reserved.
#
# This software is licensed under the terms and conditions of the FreeBSD
# License which is also known as the Simplified BSD License. You should have 
# received a copy of that license along with this software.
# 

sbin_PROGRAMS = downtimed
bin_PROGRAMS = downtimes
downtimed_SOURCES = downtimed.c downtimedb.c downtimedb.h
downtimes_SOURCES = downtimes.c downtimedb.c downtimedb.h
dist_man_MANS = downtimed.8 downtimes.1

EXTRA_DIST = README LICENSE INSTALL NEWS freebsd-startup.sh

#install-exec-hook:
#	ln -f $(DESTDIR)$(bindir)/downtimes$(EXEEXT) \
#	    $(DESTDIR)$(bindir)/downtime$(EXEEXT)

#install-data-hook:
#	ln -f $(DESTDIR)$(mandir)/man1/downtimes.1 \
#	    $(DESTDIR)$(mandir)/man1/downtime.1

.1.pdf:
	pdfroff --no-toc -man -mpdfmark $< > $@

.8.pdf:
	pdfroff --no-toc -man -mpdfmark $< > $@

# eof
