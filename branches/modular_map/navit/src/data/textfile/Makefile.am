AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../.. -shared

objdir=$(prefix)
obj_PROGRAMS=textfile.so
INSTALL_PROGRAM=/bin/true

textfile_so_SOURCES=textfile.c

textfile.so: $(textfile_so_OBJECTS)
	$(CC) $(CFLAGS) -shared -o $@ $(textfile_so_OBJECTS)
