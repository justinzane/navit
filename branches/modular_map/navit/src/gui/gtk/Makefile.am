AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../..

objdir=$(prefix)
obj_PROGRAMS=gtk.so
INSTALL_PROGRAM=/bin/true

gtk_so_SOURCES=gtkeyboard.c gui_gtk_statusbar.c gui_gtk_action.c gui_gtk_window.c

gtk.so: $(gtk_so_OBJECTS)
	$(CC) -shared -o gtk.so $(gtk_so_OBJECTS)
