#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <ctype.h>
#include "mman.h"

void * mmap_readonly_win32( const char* name, long* map_handle_ptr, long* map_file_ptr )
{
    void * mapped_ptr = NULL;
#if defined(__CEGCC__)

    wchar_t filename[MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, strlen(name), 0, 0)];
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, strlen(name), filename, wcslen(filename)) ;

    HANDLE hFile = CreateFile (filename, GENERIC_READ, FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
#else
    HANDLE hFile = CreateFile (name, GENERIC_READ, FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
#endif
    *map_file_ptr = (long)hFile;
    *map_handle_ptr = 0;

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        HANDLE hMapping = CreateFileMapping( (HANDLE)hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        mapped_ptr = MapViewOfFile(hMapping, FILE_MAP_READ, 0 , 0, 0 );
        *map_handle_ptr = (long)hMapping;
    }

    return mapped_ptr;
}

void mmap_unmap_win32( void* mem_ptr, long map_handle, long map_file )
{
    if ( mem_ptr != NULL )
    {
        UnmapViewOfFile( mem_ptr );
    }
    if ( map_handle != 0)
    {
        CloseHandle( (HANDLE)map_handle );
    }
    if ( map_file != 0  )
    {
        CloseHandle( (HANDLE)map_file );
    }
}

char *stristr(const char *String, const char *Pattern)
{
      char *pptr, *sptr, *start;

      for (start = (char *)String; *start != (int)NULL; start++)
      {
            /* find start of pattern in string */
            for ( ; ((*start!=(int)NULL) && (toupper(*start) != toupper(*Pattern))); start++)
                  ;
            if ((int)NULL == *start)
                  return NULL;

            pptr = (char *)Pattern;
            sptr = (char *)start;

            while (toupper(*sptr) == toupper(*pptr))
            {
                  sptr++;
                  pptr++;

                  /* if end of pattern then pattern was found */

                  if ((int)NULL == *pptr)
                        return (start);
            }
      }
      return NULL;
}

#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif
#ifndef SSIZE_MAX
# define SSIZE_MAX ((ssize_t) (SIZE_MAX / 2))
#endif
#if !HAVE_FLOCKFILE
# undef flockfile
# define flockfile(x) ((void) 0)
#endif
#if !HAVE_FUNLOCKFILE
# undef funlockfile
# define funlockfile(x) ((void) 0)
#endif

/* Some systems, like OSF/1 4.0 and Woe32, don't have EOVERFLOW.  */
#ifndef EOVERFLOW
# define EOVERFLOW E2BIG
#endif

/* Read up to (and including) a DELIMITER from FP into *LINEPTR (and
   NUL-terminate it).  *LINEPTR is a pointer returned from malloc (or
   NULL), pointing to *N characters of space.  It is realloc'ed as
   necessary.  Returns the number of characters read (not including
   the null terminator), or -1 on error or EOF.  */

ssize_t
getdelim (char **lineptr, size_t *n, int delimiter, FILE *fp)
{
  ssize_t result;
  size_t cur_len = 0;

  if (lineptr == NULL || n == NULL || fp == NULL)
    {
      return -1;
    }

  flockfile (fp);

  if (*lineptr == NULL || *n == 0)
    {
      *n = 120;
      *lineptr = (char *) realloc (*lineptr, *n);
      if (*lineptr == NULL)
	{
	  result = -1;
	  goto unlock_return;
	}
    }

  for (;;)
    {
      int i;

      i = getc (fp);
      if (i == EOF)
	{
	  result = -1;
	  break;
	}

      /* Make enough space for len+1 (for final NUL) bytes.  */
      if (cur_len + 1 >= *n)
	{
	  size_t needed_max =
	    SSIZE_MAX < SIZE_MAX ? (size_t) SSIZE_MAX + 1 : SIZE_MAX;
	  size_t needed = 2 * *n + 1;   /* Be generous. */
	  char *new_lineptr;

	  if (needed_max < needed)
	    needed = needed_max;
	  if (cur_len + 1 >= needed)
	    {
	      result = -1;
	      goto unlock_return;
	    }

	  new_lineptr = (char *) realloc (*lineptr, needed);
	  if (new_lineptr == NULL)
	    {
	      result = -1;
	      goto unlock_return;
	    }

	  *lineptr = new_lineptr;
	  *n = needed;
	}

      (*lineptr)[cur_len] = i;
      cur_len++;

      if (i == delimiter)
	break;
    }
  (*lineptr)[cur_len] = '\0';
  result = cur_len ? cur_len : result;

 unlock_return:
  funlockfile (fp); /* doesn't set errno */

  return result;
}

ssize_t
getline (char **lineptr, size_t *n, FILE *stream)
{
  return getdelim (lineptr, n, '\n', stream);
}
