include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = -I$(top_srcdir)/navit @NAVIT_CFLAGS@ @GTK2_CFLAGS@ -DMODULE=gui_internal
modulegui_LTLIBRARIES = libgui_internal.la
libgui_internal_la_SOURCES = gui_internal.c gui_internal.h
libgui_internal_la_LIBADD = @GTK2_LIBS@
libgui_internal_la_LDFLAGS = -module
