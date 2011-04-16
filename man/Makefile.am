SUFFIXES = .1 .1in .7 .7in
datarootdir ?= $(prefix)/share

man_MANS = xml-coreutils.7 \
	xml-awk.1 xml-cat.1 xml-cp.1 xml-cut.1 \
	xml-echo.1 xml-file.1 xml-find.1 xml-fixtags.1 \
	xml-fmt.1 xml-grep.1 xml-head.1 xml-less.1 \
	xml-ls.1 xml-mv.1 xml-printf.1 xml-rm.1 \
	xml-sed.1 xml-strings.1 xml-unecho.1 xml-wc.1 \
	xml-paste.1

EXTRA_DIST = xml-coreutils.7in \
	xml-awk.1in xml-cat.1in xml-cp.1in xml-cut.1in \
	xml-echo.1in xml-file.1in xml-find.1in xml-fixtags.1in \
	xml-fmt.1in xml-grep.1in xml-head.1in xml-less.1in \
	xml-ls.1in xml-mv.1in xml-printf.1in xml-rm.1in \
	xml-sed.1in xml-strings.1in xml-unecho.1in \
	xml-wc.1in xml-paste.1in \
	TEMPLATE.1in

.1in.1:
	cat $< \
		| sed -e "s|[@]VERSION@|$(VERSION)|g" \
		| sed -e "s|[@]PKGDATADIR@|$(pkgdatadir)|g" \
		> $@

.7in.7:
	cat $< \
		| sed -e "s|[@]VERSION@|$(VERSION)|g" \
		| sed -e "s|[@]PKGDATADIR@|$(pkgdatadir)|g" \
		> $@

CLEANFILES = $(man_MANS)

