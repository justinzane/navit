
/* speechd simple client program
 * CVS revision: $Id: speech_speech_dispatcher.c,v 1.3 2007-07-08 20:41:35 martin-s Exp $
 * Author: Tomas Cerha <cerha@brailcom.cz> */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <stdarg.h>
#include "config.h"
#include <libspeechd.h>
#include "plugin.h"
#include "speech.h"

struct speech_priv {
	SPDConnection *conn;
};

static int 
speechd_say(struct speech_priv *this, const char *text) {
	int err;

	err = spd_sayf(this->conn, SPD_MESSAGE, text);
	if (err != 1)
		return 1;
	return 0;
}

static void 
speechd_destroy(struct speech_priv *this) {
	spd_close(this->conn);
	g_free(this);
}

static struct speech_methods speechd_meth = {
	speechd_destroy,
	speechd_say,
};

static struct speech_priv *
speechd_new(char *data, struct speech_methods *meth) {
	struct speech_priv *this;
	SPDConnection *conn;

	conn = spd_open("navit","main",NULL,SPD_MODE_SINGLE);
	if (! conn) 
		return NULL;
	this=g_new(struct speech_priv,1);
	if (this) {
		this->conn=conn;
		*meth=speechd_meth;
		spd_set_punctuation(conn, SPD_PUNCT_NONE);
	}
	return this;
}


void
plugin_init(void)
{
	plugin_register_speech_type("speech_dispatcher", speechd_new);
}
