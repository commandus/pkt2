include $(top_srcdir)/Makefile.am.inc
include ../Makefile.am.inc

# the name of the plugin
PLUGIN_NAME = irda

# Non-generated sources to be scanned for registration routines
NONGENERATED_REGISTER_C_FILES = \
	packet-ircomm.c \
	packet-irda.c \
	packet-sir.c

# Non-generated sources
NONGENERATED_C_FILES = \
	$(NONGENERATED_REGISTER_C_FILES)

# Headers.
CLEAN_HEADER_FILES = \
	irda-appl.h

HEADER_FILES = \
	$(CLEAN_HEADER_FILES)

plugin_LTLIBRARIES = irda.la

irda_la_SOURCES = \
	plugin.c \
	moduleinfo.h \
	$(SRC_FILES)	\
	$(HEADER_FILES)

irda_la_CPPFLAGS = $(AM_CPPFLAGS) $(PLUGIN_CPPFLAGS)

irda_la_CFLAGS = $(AM_CFLAGS) $(PLUGIN_CFLAGS)

irda_la_LDFLAGS = $(PLUGIN_LDFLAGS)

CLEANFILES = \
	irda \
	*~

DISTCLEANFILES = \
	$(NODIST_SRC_FILES) \
	$(NODIST_HEADER_FILES)

MAINTAINERCLEANFILES = \
	Makefile.in	\
	$(GENERATED_SRC_FILES) \
	$(GENERATED_HEADER_FILES) \
	plugin.c

EXTRA_DIST = \
	plugin.rc.in		\
	CMakeLists.txt
