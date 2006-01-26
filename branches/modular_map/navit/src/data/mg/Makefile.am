AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../.. -shared

objdir=$(prefix)
obj_PROGRAMS=mg.so
INSTALL_PROGRAM=/bin/true

mg_so_SOURCES=map.c block.c town.c poly.c street.c

mg.so: $(mg_so_OBJECTS)
	$(CC) $(CFLAGS) -shared -o $@ $(mg_so_OBJECTS)
