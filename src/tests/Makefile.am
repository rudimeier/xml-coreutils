AUTOMAKE_OPTIONS = 1.4 gnits
ACLOCAL = -I ../../config
datarootdir ?= $(prefix)/share

CAT = cat-1.sh
ECHO = echo-1.sh
PRINTF = printf-1.sh
STRINGS = strings-1.sh

VERIF = cat-1.verify \
	echo-1.verify \
	printf-1.verify \
	strings-1.verify

TESTS = $(CAT) $(ECHO) $(PRINTF) $(STRINGS)

CURDIR ?= $(.CURDIR)

TESTS_ENVIRONMENT = TESTBIN=$(CURDIR)/.. DOCDIR=$(srcdir)/../../doc sourcedir=$(srcdir)

check_SCRIPTS = $(CAT) $(ECHO) $(PRINTF)

EXTRA_DIST = cat-1.shin \
	echo-1.shin \
	printf-1.shin \
	strings-1.shin \
	$(VERIF)

SUFFIXES = .shin .sh
.shin.sh:
	cat $< \
		| sed -e "s|[@]VERSION@|$(VERSION)|g" \
		> $@
	chmod +x $@
