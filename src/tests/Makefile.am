AUTOMAKE_OPTIONS = 1.4 gnits
ACLOCAL = -I ../../config
datarootdir ?= $(prefix)/share

AWK =

CAT = cat01.sh cat02.sh cat03.sh

CP = cp01.sh cp02.sh cp03.sh cp04.sh \
	cp05.sh cp06.sh cp07.sh cp08.sh \
	cp09.sh

CUT = cut01.sh cut02.sh cut03.sh

ECHO = echo01.sh echo02.sh echo03.sh echo04.sh \
	echo05.sh echo06.sh echo07.sh echo08.sh \
	echo09.sh echo10.sh

FILE = file01.sh

FIND = find01.sh find02.sh find03.sh find04.sh \
	find05.sh find06.sh find07.sh find08.sh \
	find09.sh

FIXTAGS = fixtags01.sh fixtags02.sh fixtags03.sh fixtags04.sh

FMT = fmt01.sh

GREP = grep01.sh grep02.sh grep03.sh grep04.sh \
	grep05.sh grep06.sh grep07.sh

HEAD = head01.sh head02.sh

LESS =

LS = ls01.sh ls02.sh

MV = mv01.sh mv02.sh mv03.sh

PASTE = paste01.sh

PRINTF = printf01.sh printf02.sh printf03.sh printf04.sh \
	printf05.sh printf06.sh printf07.sh

RM = rm01.sh rm02.sh rm03.sh rm04.sh

SED = sed01.sh sed02.sh sed03.sh sed04.sh \
	sed05.sh sed06.sh sed07.sh sed08.sh

STRINGS = strings01.sh strings02.sh strings03.sh

UNECHO =unecho01.sh unecho02.sh unecho03.sh

WC = wc01.sh

IDIOMS =

TESTS = $(AWK) $(CAT) $(CP) $(CUT) \
	$(ECHO) $(FILE) $(FIND) $(FIXTAGS) \
	$(FMT) $(GREP) $(HEAD) $(LESS) \
	$(LS) $(MV) $(PASTE) $(PRINTF) $(RM) \
	$(SED) $(STRINGS) $(UNECHO) $(WC) \
	$(IDIOMS)

CURDIR ?= $(.CURDIR)

TESTS_ENVIRONMENT = TESTBIN=$(CURDIR)/.. DOCDIR=$(srcdir)/../../doc sourcedir=$(srcdir)

check_SCRIPTS = $(TESTS)

CLEANFILES = $(TESTS)

EXTRA_DIST = compile_test.sh \
	cat01.testin cat02.testin cat03.testin \
	cp01.testin cp02.testin cp03.testin cp04.testin \
	cp05.testin cp06.testin cp07.testin cp08.testin \
	cp09.testin \
	cut01.testin cut02.testin cut03.testin \
	echo01.testin echo02.testin echo03.testin echo04.testin \
	echo05.testin echo06.testin echo07.testin echo08.testin \
	echo09.testin echo10.testin \
	file01.testin \
	find01.testin find02.testin find03.testin find04.testin \
	find05.testin find06.testin find07.testin find08.testin \
	find09.testin \
	fixtags01.testin fixtags02.testin fixtags03.testin fixtags04.testin \
	fmt01.testin \
	grep01.testin grep02.testin grep03.testin grep04.testin \
	grep05.testin grep06.testin grep07.testin \
	head01.testin head02.testin \
	ls01.testin ls02.testin \
	mv01.testin mv02.testin mv03.testin \
	paste01.testin \
	printf01.testin printf02.testin printf03.testin printf04.testin \
	printf05.testin printf06.testin printf07.testin \
	rm01.testin rm02.testin rm03.testin rm04.testin \
	sed01.testin sed02.testin sed03.testin sed04.testin \
	sed05.testin sed06.testin sed07.testin sed08.testin \
	strings01.testin strings02.testin strings03.testin \
	unecho01.testin unecho02.testin unecho03.testin \
	wc01.testin \
	TEMPLATE.testin

SUFFIXES = .testin .sh
.testin.sh:
	cat $< | /bin/sh $(CURDIR)/compile_test.sh > $@
	chmod +x $@
