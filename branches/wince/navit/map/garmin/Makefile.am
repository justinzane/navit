include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@   -I$(top_srcdir)/navit -DMODULE=map_garmin
AM_CPPFLAGS+= @LIBGARMIN_CFLAGS@
INCLUDES=-I$(top_srcdir)/navit/map/garmin/libgarmin
AM_CPPFLAGS+=-I$(top_srcdir)/navit/map/garmin/libgarmin
AM_CPPFLAGS+=-I$(srcdir)

modulemap_LTLIBRARIES = libmap_garmin.la
libmap_garmin_la_SOURCES = garmin.c garmin.h gar2navit.c gar2navit.h
libmap_garmin_la_LIBADD = @LIBGARMIN_LIBS@
libmap_garmin_la_LDFLAGS = -module -avoid-version
nodist_libmap_garmin_la_SOURCES = g2nbuiltin.h
builddir = @builddir@

gar2navit.l$(OBJEXT): g2nbuiltin.h

CLEANFILES = g2nbuiltin.h

noinst_PROGRAMS=gentypes$(EXEEXT)
gentypes_SOURCES=gentypes.c

gentypes.$(OBJEXT): gentypes.c
	$(CC_FOR_BUILD) -I$(top_srcdir)/navit -c -o $@ $<

gentypes$(EXEEXT): $(gentypes_OBJECTS) $(gentypes_DEPENDENCIES) 
	@rm -f gentypes$(EXEEXT)
	$(CCLD_FOR_BUILD) -o $@ $(gentypes_OBJECTS)

g2nbuiltin.h: gentypes$(EXEEXT) garmintypes.txt
	./gentypes$(EXEEXT) $(top_srcdir)/navit/map/garmin/garmintypes.txt $@

EXTRA_DIST = garmintypes.txt

