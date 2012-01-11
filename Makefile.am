AUTOMAKE_OPTIONS = 1.4 gnits
datarootdir ?= $(prefix)/share

ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = doc src man

EXTRA_DIST = SFX

sfx:
	make dist && (cat SFX $(distdir).tar.gz > $(distdir).sfx.sh)
	test -e $(distdir).sfx.sh && chmod +x $(distdir).sfx.sh
