AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../..

objdir=$(prefix)
obj_PROGRAMS=gtk_drawing_area.so
INSTALL_PROGRAM=/bin/true

gtk_drawing_area_so_SOURCES=graphics_gtk_drawing_area.c

gtk_drawing_area.so: $(gtk_drawing_area_so_OBJECTS)
	$(CC) -shared -o gtk_drawing_area.so $(gtk_drawing_area_so_OBJECTS)
