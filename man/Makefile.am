SUFFIXES = .1 .1in
man_MANS = xml-coreutils.7 xml-cat.1 xml-echo.1 xml-printf.1 xml-less.1
EXTRA_DIST = xml-coreutils.7in xml-cat.1in xml-echo.1in xml-printf.1in xml-less.1in

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

CLEANFILES = xml-coreutils.7 xml-cat.1 xml-echo.1 xml-printf.1 xml-less.1
