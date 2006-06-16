# note . occurs before tests
SUBDIRS = . tests

AM_CFLAGS = -funsigned-char -Wall -pedantic -O3

COMMON = error.h error.c signal.h signal.c common.h 
PARSER = parser.h parser.c
IO = io.h io.c
STDOUT = stdout.h stdout.c
WRAP = wrap.h wrap.c
XPATH = xpath.h xpath.c 
XMATCH = xmatch.h xmatch.c
MEM = mem.h mem.c
COLLECT = tempcollect.h tempcollect.c
STRING = stringlist.h stringlist.c
FORMAT = format.h format.c
ENTITIES = entities.h entities.c
BLOCKS = blockmgr.h blockmgr.c fbreader.h fbreader.c fbparser.h fbparser.c
CURSOR = cursor.h cursor.c cursormgr.h cursormgr.c skip.h skip.c cursorrep.h cursorrep.c
ATTRIBUTES = attributes.h attributes.c
LESSUI = lessui.h lessui.c lessdisp.h lessdisp.c lessrend.h lessrend.c
HASH = jenkins.c

bin_PROGRAMS = xml-cat xml-printf xml-echo xml-less
xml_cat_SOURCES = xml-cat.c $(COMMON) $(PARSER) $(IO) $(STDOUT) $(WRAP)

xml_printf_SOURCES = xml-printf.c $(COMMON) $(PARSER) $(IO) $(STDOUT) $(XPATH) $(XMATCH) $(MEM) $(COLLECT) $(STRING) $(FORMAT) 

xml_echo_SOURCES = xml-echo.c $(COMMON) $(WRAP) $(IO) $(STDOUT) $(FORMAT) $(XPATH) $(MEM) $(ENTITIES) $(PARSER)

xml_less_SOURCES = xml-less.c $(COMMON) $(IO) $(MEM) $(BLOCKS) $(STDOUT) $(CURSOR) $(PARSER) $(LESSUI) $(ENTITIES) $(ATTRIBUTES) $(HASH) $(XPATH)
xml_less_LDADD = -lslang

check_PROGRAMS = 
