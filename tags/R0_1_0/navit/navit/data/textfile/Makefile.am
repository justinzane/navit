include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=data_textfile
if PLUGINS
  moduledata_LTLIBRARIES = libdata_textfile.la
else
  noinst_LTLIBRARIES = libdata_textfile.la
endif
libdata_textfile_la_SOURCES = textfile.c textfile.h
