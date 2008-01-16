SUFFIXES = .1 .1in .7 .7in
datarootdir ?= $(prefix)/share

man_MANS = xml-coreutils.7 xml-cat.1 xml-echo.1 xml-printf.1 xml-less.1 \
	xml-strings.1 xml-ls.1 xml-find.1
EXTRA_DIST = xml-coreutils.7in xml-cat.1in xml-echo.1in xml-printf.1in xml-less.1in \
	xml-strings.1in xml-ls.1in xml-find.1in

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

CLEANFILES = xml-coreutils.7 xml-cat.1 xml-echo.1 xml-printf.1 xml-less.1 \
	xml-strings.1 xml-ls.1 xml-find.1
