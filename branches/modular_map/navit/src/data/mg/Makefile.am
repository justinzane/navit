AUTOMAKE_OPTIONS = no-exeext
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../..

objdir=$(prefix)
obj_PROGRAMS=mg.o
INSTALL_PROGRAM=/bin/true

mg_o_SOURCES=map.c block.c town.c poly.c street.c

mg.o: $(mg_o_OBJECTS)
	ld -r -o mg.o $(mg_o_OBJECTS)
