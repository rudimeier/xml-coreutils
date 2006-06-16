AUTOMAKE_OPTIONS = 1.4 gnits
ACLOCAL = -I ../../config


TESTS = 
TESTS_ENVIRONMENT = TESTBIN=$(CURDIR)/.. DOCDIR=$(srcdir)/../../doc sourcedir=$(srcdir)

check_SCRIPTS = 

EXTRA_DIST = 

SUFFIXES = .shin .sh
.shin.sh:
	cat $< \
		| sed -e "s|[@]VERSION@|$(VERSION)|g" \
		> $@
	chmod +x $@
