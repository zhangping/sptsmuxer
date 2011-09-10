plugin_LTLIBRARIES = libgstsptsmuxer.la

# sources used to compile this plug-in
libgstsptsmuxer_la_SOURCES = sptsmuxer.c \
	psi.c \
	pes.c \
	spts.c 

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstsptsmuxer_la_CFLAGS = $(GST_CFLAGS)
libgstsptsmuxer_la_LIBADD = $(GST_LIBS)
libgstsptsmuxer_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstsptsmuxer_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = sptsmuxer.h \
	psi.h \
	pes.h \
	spts.h \
	crc.h
