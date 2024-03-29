#include <time.h>
#include <glib.h>
#include "coord.h"
#include "item.h"
#include "route.h"
#include "speech.h"
#include "phrase.h"

void
phrase_route_calc(void *speech)
{
#if 0
	if (! speech)
		return;
	speech_say(speech,"Die Route wird berechnet\n");	
#endif
}

void
phrase_route_calculated(void *speech, void *route)
{
#if 0
	struct tm *eta;
#endif
	if (! speech)
		return;

#if 0 /* FIXME */
        eta=route_get_eta(route);

        speech_sayf(speech,"Die Route wurde berechnet, geschätzte Ankunftszeit %d Uhr %d  Entfernung %4.0f Kilometer", eta->tm_hour,eta->tm_min,route_get_len(route)/1000);
#endif

}
