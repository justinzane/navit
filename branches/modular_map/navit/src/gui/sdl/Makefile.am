include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @PACKAGE_CFLAGS@ -I../.. -I/usr/include/CEGUI -I/usr/include/freetype2 -I/usr/include/SDL
plugin_LTLIBRARIES = libgui_sdl.la
libgui_sdl_la_SOURCES = gui_sdl_window.cpp
libgui_sdl_la_LIBADD = -lSDL -lCEGUIBase -lCEGUIOpenGLRenderer -lGLC
