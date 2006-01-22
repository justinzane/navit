SUBDIRS=fib-1.0 data gui graphics

fib-1.0 fib-1.0/Makefile.in: fib-1.0/Makefile.am
	cd fib-1.0
	./configure

AM_CPPFLAGS = @PACKAGE_CFLAGS@


bin_PROGRAMS = navit

navit_SOURCES = compass.c coord.c country.c cursor.c data_window.c debug.c destination.c \
	display.c file.c graphics.c item.c log.c main.c map.c \
	mapset.c menu.c navigation.c param.c phrase.c plugin.c popup.c \
	profile.c python.c route.c search.c speech.c transform.c track.c \
	util.c vehicle.c block.h compass.h config.h container.h coord.h country.h \
	cursor.h data.h data_window.h data_window_int.h debug.h destination.h display.h draw_info.h \
	file.h graphics.h gtkext.h log.h main.h map-share.h map.h map_data.h mapset.h menu.h navigation.h \
	param.h phrase.h plugin.h point.h popup.h route.h search.h speech.h statusbar.h \
	toolbar.h transform.h track.h util.h vehicle.h

navit_LDADD = @PACKAGE_LIBS@ gui/gtk/gtk.o graphics/gtk_drawing_area/gtk_drawing_area.o data/mg/mg.o -Lfib-1.0 -lfib

