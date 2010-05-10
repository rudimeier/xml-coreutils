AUTOMAKE_OPTIONS = 1.4 gnits
ACLOCAL = -I config
datarootdir ?= $(prefix)/share

ACLOCAL_AMFLAGS = -I m4 --install

AUX_DIST = config/config.guess \
	config/config.sub \
	config/depcomp \
	config/install-sh \
	config/missing

SUBDIRS = doc src man

dist-hook:
	(cd $(distdir) && mkdir -p config)
	for file in $(AUX_DIST); do \
		cp $$file $(distdir)/$$file; \
	done
