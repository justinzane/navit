include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -I$(top_srcdir)/navit -DMODULE=vehicle_gpsd
modulevehicle_LTLIBRARIES = libvehicle_gpsd.la
libvehicle_gpsd_la_SOURCES = vehicle_gpsd.c
libvehicle_gpsd_la_LIBADD = @GPSD_LIBS@
