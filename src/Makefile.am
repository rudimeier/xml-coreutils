# note . occurs before tests
SUBDIRS = . tests
datarootdir ?= $(prefix)/share


#AM_CFLAGS = -funsigned-char -Wall -pedantic -O3
AM_CFLAGS = -funsigned-char -Wall -pedantic
AM_YFLAGS = -d

COMMON = myerror.h myerror.c mysignal.h mysignal.c common.h common.c
PARSER = parser.h parser.c
IO = io.h io.c
STDOUT = stdout.h stdout.c
WRAP = wrap.h wrap.c
TEMPF = tempfile.h tempfile.c
XPATH = xpath.h xpath.c 
LF = leafnode.h leafnode.c
XMATCH = xmatch.h xmatch.c
XPRED = xpredicate.h xpredicate.c
XATT = xattribute.h xattribute.c
SMATCH = smatch.h smatch.c
SYM = symbols.h symbols.c
MEM = mem.h mem.c
OBJSTACK = objstack.h objstack.c
COLLECT = tempcollect.h tempcollect.c
VAR = tempvar.h tempvar.c
CSTRING = cstring.h cstring.c
STRLST = stringlist.h stringlist.c
FILELST = filelist.h filelist.c
FORMAT = format.h format.c
ENTITIES = entities.h entities.c
BLOCKS = blockmgr.h blockmgr.c fbreader.h fbreader.c fbparser.h fbparser.c
CURSOR = cursor.h cursor.c cursormgr.h cursormgr.c skip.h skip.c cursorrep.h cursorrep.c
ATTLST = attlist.h attlist.c
LESSUI = lessui.h lessui.c lessdisp.h lessdisp.c lessrend.h lessrend.c
HASH = jenkins.c
STDPARSE = stdparse.h stdparse.c
STDPRINT = stdprint.h stdprint.c
STDSEL = stdselect.h stdselect.c
LFPARSE = leafparse.h leafparse.c
ECHOC = echo.h echo.c
UNECHO = unecho.h unecho.c
SEDC = sed.h sed.c
RCM = rcm.h rcm.c
ROLLBACK = rollback.h rollback.c
NHIST = nhistory.h nhistory.c
INTERVAL = interval.h interval.c
HTFILT = htfilter.h htfilter.c
AWKMEM = awkmem.h awkmem.c 
AWKAST = awkast.h awkast.c
AWKVM = awkl.l awkp.y awkvm.h awkvm.c

STDCOMMON = $(COMMON) $(PARSER) $(FILELST) $(IO) $(STDOUT) $(MEM) $(ENTITIES) $(CSTRING)
STDPARSING = $(STDPARSE) $(STDSEL) $(XPATH) $(XMATCH) $(XPRED) $(XATT) $(HASH) $(OBJSTACK) $(NHIST)
LEAFPARSING = $(LFPARSE) $(STDSEL) $(XPATH) $(XMATCH) $(XPRED) $(XATT) $(LF) $(HASH) $(OBJSTACK) $(NHIST) 

bin_PROGRAMS = xml-cat xml-printf xml-echo xml-less xml-strings xml-ls \
	xml-file xml-find xml-grep xml-fmt xml-wc xml-cut xml-head \
	xml-unecho xml-sed xml-rm xml-cp xml-mv xml-fixtags xml-paste xml-awk

xml_cat_SOURCES = xml-cat.c $(STDCOMMON) $(WRAP)

xml_printf_SOURCES = xml-printf.c $(STDCOMMON) $(STDPARSING) $(COLLECT) $(STRLST) $(FORMAT) $(VAR)

xml_echo_SOURCES = xml-echo.c $(COMMON) $(WRAP) $(IO) $(STDOUT) $(FORMAT) $(XPATH) $(MEM) $(PARSER) $(ECHOC) $(ENTITIES) $(HASH) $(COLLECT) $(VAR) $(CSTRING)

xml_less_SOURCES = xml-less.c $(COMMON) $(IO) $(MEM) $(BLOCKS) $(STDOUT) $(CURSOR) $(PARSER) $(LESSUI) $(ENTITIES) $(ATTLST) $(HASH) $(XPATH) $(CSTRING) $(TEMPF) $(STRLST)
xml_less_LDADD = -lslang -lncurses

xml_strings_SOURCES = xml-strings.c $(STDCOMMON) $(STDPARSING)

xml_ls_SOURCES = xml-ls.c $(STDCOMMON) $(STDPARSING) $(WRAP)  

xml_find_SOURCES = xml-find.c $(STDCOMMON) $(STDPARSING) $(STRLST) $(COLLECT) $(WRAP) $(TEMPF)

xml_grep_SOURCES = xml-grep.c $(STDCOMMON) $(STDPARSING) $(CURSOR) $(SMATCH) $(BLOCKS) $(COLLECT) $(WRAP) $(VAR) $(INTERVAL) $(STRLST)

xml_fmt_SOURCES = xml-fmt.c $(STDCOMMON) $(STDPARSING) $(STDPRINT)

xml_wc_SOURCES = xml-wc.c $(STDCOMMON) $(STDPARSING) 

xml_cut_SOURCES = xml-cut.c $(STDCOMMON) $(STDPARSING) $(WRAP) $(STDPRINT) $(COLLECT) $(INTERVAL)

xml_head_SOURCES = xml-head.c $(STDCOMMON) $(STDPARSING) $(STDPRINT)

xml_unecho_SOURCES = xml-unecho.c $(STDCOMMON) $(LEAFPARSING) $(VAR) $(COLLECT) $(UNECHO)

xml_sed_SOURCES = xml-sed.c $(STDCOMMON) $(LEAFPARSING) $(VAR) $(COLLECT) $(UNECHO) $(SEDC) $(ECHOC) $(WRAP) $(FORMAT) $(STRLST)

xml_rm_SOURCES = xml-rm.c $(STDCOMMON) $(STDPARSING) $(STDPRINT) $(RCM) $(ROLLBACK) $(COLLECT) $(WRAP) $(TEMPF) $(STRLST)

xml_cp_SOURCES = xml-cp.c $(STDCOMMON) $(STDPARSING) $(STDPRINT) $(COLLECT) $(RCM) $(ROLLBACK) $(WRAP) $(TEMPF) $(STRLST)

xml_mv_SOURCES = xml-mv.c $(STDCOMMON) $(STDPARSING) $(STDPRINT) $(RCM) $(ROLLBACK) $(COLLECT) $(WRAP) $(TEMPF) $(STRLST)

xml_fixtags_SOURCES = xml-fixtags.c $(STDCOMMON) $(XPATH) $(WRAP) $(OBJSTACK) $(STRLST) $(HTFILT)

xml_file_SOURCES = xml-file.c $(STDCOMMON) $(STDPARSING) 

xml_paste_SOURCES = xml-paste.c $(STDCOMMON) $(STDPARSING) $(COLLECT) $(TEMPF) $(STRLST) $(WRAP)

xml_awk_SOURCES = xml-awk.c $(STDCOMMON) $(STDPARSING) $(VAR) $(COLLECT) $(AWKVM) $(AWKMEM) $(AWKAST) $(SYM)
xml_awk_LDADD = @LEXLIB@

check_PROGRAMS = 

