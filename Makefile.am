#
# Makefile.am
#
# Copyright (c) 2009-2016 Janne Snabb. All rights reserved.
#
# This software is licensed under the terms and conditions of the
# Simplified BSD License. You should have received a copy of that
# license along with this software.
#

sbin_PROGRAMS = downtimed
bin_PROGRAMS = downtimes
downtimed_SOURCES = downtimed.c downtimedb.c downtimedb.h
downtimes_SOURCES = downtimes.c downtimedb.c downtimedb.h
dist_man_MANS = downtimed.8 downtimes.1

EXTRA_DIST = README.md LICENSE INSTALL NEWS startup-scripts

install-exec-hook:
	ln -f $(DESTDIR)$(bindir)/downtimes$(EXEEXT) \
	    $(DESTDIR)$(bindir)/downtime$(EXEEXT)

install-data-hook:
	ln -f $(DESTDIR)$(mandir)/man1/downtimes.1 \
	    $(DESTDIR)$(mandir)/man1/downtime.1

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/downtime$(EXEEXT)
	rm -f $(DESTDIR)$(mandir)/man1/downtime.1

.1.pdf:
	pdfroff --no-toc -man -mpdfmark $< > $@

.8.pdf:
	pdfroff --no-toc -man -mpdfmark $< > $@

# eof
