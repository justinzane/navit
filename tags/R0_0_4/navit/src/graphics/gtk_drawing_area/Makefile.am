include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @GTK2_CFLAGS@ @FREETYPE2_CFLAGS@ @FONTCONFIG_CFLAGS@ -I$(top_srcdir)/src -DMODULE=graphics_gtk_drawing_area
modulegraphics_LTLIBRARIES = libgraphics_gtk_drawing_area.la
libgraphics_gtk_drawing_area_la_SOURCES = graphics_gtk_drawing_area.c
libgraphics_gtk_drawing_area_la_LDFLAGS = @GTK2_LIBS@ @FREETYPE2_LIBS@ @FONTCONFIG_LIBS@
