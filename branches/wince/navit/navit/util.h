#ifndef NAVIT_types_H
#define NAVIT_types_H

#include <ctype.h>
#include <windows.h>
#include <wchar.h>
#include <glib.h>

void strtoupper(char *dest, const char *src);
void strtolower(char *dest, const char *src);
GList * g_hash_to_list(GHashTable *h);

TCHAR* newSysString(const char *);


#define UTF8_TO_UNICODE(UTF8NAME,UNICODENAME) \
    int UTF8_LENGTH = strlen(UTF8NAME); \
    wchar_t UNICODENAME[UTF8_LENGTH + 1]; \
    mbstowcs(UNICODENAME, UTF8NAME, UTF8_LENGTH + 1);

#define UNICODE_TO_UTF8(UNICODENAME,UTF8NAME) \
    int UNICODE_LENGTH = lstrlen(UNICODENAME); \
    char UTF8NAME[UNICODE_LENGTH + 1]; \
    wcstombs(UTF8NAME, UNICODENAME, UNICODE_LENGTH + 1);


#ifdef _UNICODE

#define _sntprintf _snwprintf
#define stprintf swprintf

#define TCHAR_TO_UTF8(PARAM1,PARAM2) UNICODE_TO_UTF8(PARAM1,PARAM2)
#define UTF8_TO_TCHAR(PARAM1,PARAM2) UTF8_TO_UNICODE(PARAM1,PARAM2)


#else //_UNICODE

#define TCHAR_TO_UTF8(PARAM1,PARAM2) \
    char *PARAM2 = PARAM1;

#define UTF8_TO_TCHAR(PARAM1,PARAM2) \
    char *PARAM2 = PARAM1;


#define _sntprintf _snprintf
#define stprintf sprintf

#endif //_UNICODE

#endif

