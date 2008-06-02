#include <glib.h>
#include <ctype.h>
#include "util.h"

void
strtoupper(char *dest, const char *src)
{
	while (*src)
		*dest++=toupper(*src++);
	*dest='\0';
}

void
strtolower(char *dest, const char *src)
{
	while (*src)
		*dest++=tolower(*src++);
	*dest='\0';
}


static void
hash_callback(gpointer key, gpointer value, gpointer user_data)
{
	GList **l=user_data;
	*l=g_list_prepend(*l, value);
}

GList *
g_hash_to_list(GHashTable *h)
{
	GList *ret=NULL;
	g_hash_table_foreach(h, hash_callback, &ret);

	return ret;
}


#if defined(_UNICODE)
wchar_t* newSysString(const char *toconvert)
{
    int newstrlen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, toconvert, -1, 0, 0);
    wchar_t *newstring = g_new(wchar_t,newstrlen);
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, toconvert, -1, newstring, newstrlen) ;
    return newstring;
}
#else
char * newSysString(const char *toconvert)
{
    char *newstring = g_new(byte,strlen(toconvert)+1);
    strcpy(newstring, toconvert);
    return newstring;
}
#endif

