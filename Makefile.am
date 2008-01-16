AUTOMAKE_OPTIONS = 1.4 gnits
ACLOCAL = -I config
datarootdir ?= $(prefix)/share


AUX_DIST = config/config.guess \
	config/config.sub \
	config/install-sh \
	config/missing \
	config/mkinstalldirs

EXTRA_DIST = bootstrap

SUBDIRS = doc src man

dist-hook:
	(cd $(distdir) && mkdir -p config)
	for file in $(AUX_DIST); do \
		cp $$file $(distdir)/$$file; \
	done
