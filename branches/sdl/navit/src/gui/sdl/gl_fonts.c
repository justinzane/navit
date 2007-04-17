/*
 * This code was created by Jeff Molofee '99 
 * (ported to Linux/SDL by Ti Leggett '01)
 *
 * If you've found this code useful, please let me know.
 *
 * Visit Jeff at http://nehe.gamedev.net/
 * 
 * or for port-specific comments, questions, bugreports etc. 
 * email to leggett@eecs.tulane.edu
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include "SDL/SDL.h"

const FONT_CHAR_MEMSIZE=256;

GLuint  base;
GLuint  osd_font;

/* function to recover memory form our list of characters */
GLvoid KillFont( GLvoid )
{
    glDeleteLists( base, FONT_CHAR_MEMSIZE );

    return;
}

/* function to release/destroy our resources and restoring the old desktop */
void Quit( int returnCode )
{

    KillFont( );
    /* clean up the window */
    SDL_Quit( );

    /* and exit appropriately */
    exit( returnCode );
}

/* function to build our font list */
GLvoid buildFont( GLvoid )
{
    Display *dpy;          /* Our current X display */
    XFontStruct *fontInfo; /* Our font info */

    /* Storage for characters */
    base = glGenLists( FONT_CHAR_MEMSIZE );
	osd_font = glGenLists( FONT_CHAR_MEMSIZE );

    /* Get our current display long enough to get the fonts */
    dpy = XOpenDisplay( NULL );

// 
    /* Get the font information */
    fontInfo = XLoadQueryFont( dpy, "-adobe-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*" );

    /* If the above font didn't exist try one that should */
    if ( fontInfo == NULL )
	{
	    fontInfo = XLoadQueryFont( dpy, "fixed" );
	    /* If that font doesn't exist, something is wrong */
	    if ( fontInfo == NULL )
		{
		    fprintf( stderr, "no X font available?\n" );
		    Quit( 1 );
		}
	}

    /* generate the list */
    glXUseXFont( fontInfo->fid, 32, FONT_CHAR_MEMSIZE, base );

    /* Get the font information */
    fontInfo = XLoadQueryFont( dpy, "-adobe-helvetica-medium-r-*-*-24-*-*-*-*-*-*-*" );

    /* If the above font didn't exist try one that should */
    if ( fontInfo == NULL )
	{
	printf("Using fallback font #2\n");
	    fontInfo = XLoadQueryFont( dpy, "fixed" );
	    /* If that font doesn't exist, something is wrong */
	    if ( fontInfo == NULL )
		{
		    fprintf( stderr, "no X font available?\n" );
		    Quit( 1 );
		}
	}

    /* generate the list */
    glXUseXFont( fontInfo->fid, 32, FONT_CHAR_MEMSIZE, osd_font );


    /* Recover some memory */
    XFreeFont( dpy, fontInfo );

    /* close the display now that we're done with it */
    XCloseDisplay( dpy );

    return;
}

/* Print our GL text to the screen */
GLvoid glPrint( const char *fmt, ... )
{
    char text[256]; /* Holds our string */
    va_list ap;     /* Pointer to our list of elements */

    /* If there's no text, do nothing */
    if ( fmt == NULL )
	return;

    /* Parses The String For Variables */
    va_start( ap, fmt );
      /* Converts Symbols To Actual Numbers */
      vsprintf( text, fmt, ap );
    va_end( ap );

    /* Pushes the Display List Bits */
    glPushAttrib( GL_LIST_BIT );

    /* Sets base character to 32 */
    glListBase( base - 32 );

    /* Draws the text */
    glCallLists( strlen( text ), GL_UNSIGNED_BYTE, text );

    /* Pops the Display List Bits */
    glPopAttrib( );
}

/* Print our GL text to the screen */
GLvoid glPrintBig( const char *fmt, ... )
{
    char text[256]; /* Holds our string */
    va_list ap;     /* Pointer to our list of elements */

    /* If there's no text, do nothing */
    if ( fmt == NULL )
	return;

    /* Parses The String For Variables */
    va_start( ap, fmt );
      /* Converts Symbols To Actual Numbers */
      vsprintf( text, fmt, ap );
    va_end( ap );

    /* Pushes the Display List Bits */
    glPushAttrib( GL_LIST_BIT );

    /* Sets base character to 32 */
    glListBase( osd_font - 32 );

    /* Draws the text */
    glCallLists( strlen( text ), GL_UNSIGNED_BYTE, text );

    /* Pops the Display List Bits */
    glPopAttrib( );
}



