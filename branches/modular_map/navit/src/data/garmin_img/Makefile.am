AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../.. -shared

objdir=$(prefix)
obj_PROGRAMS=garmin_img.so
INSTALL_PROGRAM=/bin/true

garmin_img_so_SOURCES=garmin_img.c

garmin_img.so: $(garmin_img_so_OBJECTS)
	$(CC) $(CFLAGS) -shared -o $@ $(garmin_img_so_OBJECTS)
