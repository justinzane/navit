#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <glib.h>
#include <sys/stat.h>
#include <termios.h>
#include <math.h>
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "vehicle.h"

static void vehicle_file_disable_watch(struct vehicle_priv *priv);
static void vehicle_file_enable_watch(struct vehicle_priv *priv);

enum file_type {
	file_type_pipe = 1, file_type_device, file_type_file
};

static int buffer_size = 256;

struct vehicle_priv {
	char *source;
	enum file_type file_type;
	struct callback_list *cbl;
	int fd;
	FILE *file;
	guint watch;
	GIOChannel *iochan;
	char *buffer;
	int buffer_pos;

	struct coord_geo geo;
	double speed;
	double direction;
	double height;
	int status;
	int sats_used;
};

static int
vehicle_file_open(struct vehicle_priv *priv)
{
	char *name;
	struct stat st;
	struct termios tio;

	name = priv->source + 5;
	if (!strncmp(priv->source, "file:", 5)) {
		priv->fd = open(name, O_RDONLY | O_NDELAY);
		if (priv->fd < 0)
			return 0;
		stat(name, &st);
		if (S_ISREG(st.st_mode)) {
			priv->file_type = file_type_file;
		} else {
			tcgetattr(priv->fd, &tio);
			cfmakeraw(&tio);
			cfsetispeed(&tio, B4800);
			cfsetospeed(&tio, B4800);
			tio.c_cc[VMIN] = 16;
			tio.c_cc[VTIME] = 1;
			tcsetattr(priv->fd, TCSANOW, &tio);
			priv->file_type = file_type_device;
		}
	} else {
		priv->file = popen(name, "r");
		if (!priv->file)
			return 0;
		priv->fd = fileno(priv->file);
		priv->file_type = file_type_pipe;
	}
	priv->iochan = g_io_channel_unix_new(priv->fd);
	return 1;
}

static void
vehicle_file_close(struct vehicle_priv *priv)
{
	GError *error = NULL;
	if (priv->iochan) {
		g_io_channel_shutdown(priv->iochan, 0, &error);
		priv->iochan = NULL;
	}
	if (priv->file)
		pclose(priv->file);
	else if (priv->fd >= 0)
		close(priv->fd);
	priv->file = NULL;
	priv->fd = -1;
}

static int
vehicle_file_enable_watch_timer(gpointer t)
{
	struct vehicle_priv *priv = t;
	vehicle_file_enable_watch(priv);
	dbg(1, "enter\n");

	return FALSE;
}


static void
vehicle_file_parse(struct vehicle_priv *priv, char *buffer)
{
	char *p, *item[16];
	double lat, lng;
	int i, bcsum;
	int len = strlen(buffer);
	unsigned char csum = 0;

	dbg(1, "buffer='%s'\n", buffer);
	for (;;) {
		if (len < 4) {
			dbg(0, "too short\n");
			return;
		}
		if (buffer[len - 1] == '\r' || buffer[len - 1] == '\n')
			buffer[--len] = '\0';
		else
			break;
	}
	if (buffer[0] != '$') {
		dbg(0, "no leading $\n");
		return;
	}
	if (buffer[len - 3] != '*') {
		dbg(0, "no *XX\n");
		return;
	}
	for (i = 1; i < len - 3; i++) {
		csum ^= (unsigned char) (buffer[i]);
	}
	if (!sscanf(buffer + len - 2, "%x", &bcsum)) {
		dbg(0, "no checksum\n");
		return;
	}
	if (bcsum != csum) {
		dbg(0, "wrong checksum\n");
		return;
	}

	i = 0;
	p = buffer;
	while (i < 16) {
		item[i++] = p;
		while (*p && *p != ',')
			p++;
		if (!*p)
			break;
		*p++ = '\0';
	}

	if (!strncmp(buffer, "$GPGGA", 6)) {
		/*                                                           1 1111
		   0      1          2         3 4          5 6 7  8   9     0 1234
		   $GPGGA,184424.505,4924.2811,N,01107.8846,E,1,05,2.5,408.6,M,,,,0000*0C
		   UTC of Fix[1],Latitude[2],N/S[3],Longitude[4],E/W[5],Quality(0=inv,1=gps,2=dgps)[6],Satelites used[7],
		   HDOP[8],Altitude[9],"M"[10],height of geoid[11], "M"[12], time since dgps update[13], dgps ref station [14] 
		 */
		sscanf(item[2], "%lf", &lat);
		priv->geo.lat = floor(lat / 100);
		lat -= priv->geo.lat * 100;
		priv->geo.lat += lat / 60;

		sscanf(item[4], "%lf", &lng);
		priv->geo.lng = floor(lng / 100);
		lng -= priv->geo.lng * 100;
		priv->geo.lng += lng / 60;

		sscanf(item[6], "%d", &priv->status);
		sscanf(item[7], "%d", &priv->sats_used);
		sscanf(item[9], "%lf", &priv->height);

		callback_list_call_0(priv->cbl);
		if (priv->file_type == file_type_file) {
			vehicle_file_disable_watch(priv);
			g_timeout_add(1000,
				      vehicle_file_enable_watch_timer,
				      priv);
		}
	}
	if (!strncmp(buffer, "$GPVTG", 6)) {
		/* 0      1      2 34 5    6 7   8 
		   $GPVTG,143.58,T,,M,0.26,N,0.5,K*6A 
		   Course Over Ground Degrees True[1],"T"[2],Course Over Ground Degrees Magnetic[3],"M"[4],
		   Speed in Knots[5],"N"[6],"Speed in KM/H"[7],"K"[8]
		 */
		sscanf(item[1], "%lf", &priv->direction);
		sscanf(item[7], "%lf", &priv->speed);
	}
	if (!strncmp(buffer, "$GPRMC", 6)) {
		/*                                                           1     1
		   0      1      2 3        4 5         6 7     8     9      0     1
		   $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A 
		   Time[1],Active/Void[2],lat[3],N/S[4],long[5],W/E[6],speed in knots[7],track angle[8],date[9],
		   magnetic variation[10],magnetic variation direction[11]
		 */
		sscanf(item[8], "%lf", &priv->direction);
		sscanf(item[7], "%lf", &priv->speed);
		priv->speed *= 1.852;
	}
}

static gboolean
vehicle_file_io(GIOChannel * iochan, GIOCondition condition, gpointer t)
{
	struct vehicle_priv *priv = t;
	int size;
	char *str, *tok;

	dbg(1, "enter condition=%d\n", condition);
	if (condition == G_IO_IN) {
		size =
		    read(g_io_channel_unix_get_fd(iochan),
			 priv->buffer + priv->buffer_pos,
			 buffer_size - priv->buffer_pos - 1);
		if (size <= 0) {
			vehicle_file_close(priv);
			vehicle_file_open(priv);
			return TRUE;
		}
		priv->buffer_pos += size;
		priv->buffer[priv->buffer_pos] = '\0';
		dbg(1, "size=%d pos=%d buffer='%s'\n", size,
		    priv->buffer_pos, priv->buffer);
		str = priv->buffer;
		while ((tok = index(str, '\n'))) {
			*tok++ = '\0';
			dbg(1, "line='%s'\n", str);
			vehicle_file_parse(priv, str);
			str = tok;
		}
		if (str != priv->buffer) {
			size = priv->buffer + priv->buffer_pos - str;
			memmove(priv->buffer, str, size + 1);
			priv->buffer_pos = size;
			dbg(1, "now pos=%d buffer='%s'\n",
			    priv->buffer_pos, priv->buffer);
		} else if (priv->buffer_pos == buffer_size - 1) {
			dbg(0,
			    "Overflow. Most likely wrong baud rate or no nmea protocol\n");
			priv->buffer_pos = 0;
		}
		return TRUE;
	}
	return FALSE;
}

static void
vehicle_file_enable_watch(struct vehicle_priv *priv)
{
	priv->watch =
	    g_io_add_watch(priv->iochan, G_IO_IN | G_IO_ERR | G_IO_HUP,
			   vehicle_file_io, priv);
}

static void
vehicle_file_disable_watch(struct vehicle_priv *priv)
{
	if (priv->watch)
		g_source_remove(priv->watch);
	priv->watch = 0;
}


static void
vehicle_file_destroy(struct vehicle_priv *priv)
{
	vehicle_file_close(priv);
	if (priv->source)
		g_free(priv->source);
	if (priv->buffer)
		g_free(priv->buffer);
	g_free(priv);
}

static int
vehicle_file_position_attr_get(struct vehicle_priv *priv,
			       enum attr_type type, struct attr *attr)
{
	switch (type) {
	case attr_position_height:
		attr->u.numd = &priv->height;
		break;
	case attr_position_speed:
		attr->u.numd = &priv->speed;
		break;
	case attr_position_direction:
		attr->u.numd = &priv->direction;
		break;
	case attr_position_sats_used:
		attr->u.num = priv->sats_used;
		break;
	case attr_position_coord_geo:
		attr->u.coord_geo = &priv->geo;
		break;
	default:
		return 0;
	}
	attr->type = type;
	return 1;
}

struct vehicle_methods vehicle_file_methods = {
	vehicle_file_destroy,
	vehicle_file_position_attr_get,
};

static struct vehicle_priv *
vehicle_file_new_file(struct vehicle_methods
		      *meth, struct callback_list
		      *cbl, struct attr **attrs)
{
	struct vehicle_priv *ret;
	struct attr *source;

	dbg(1, "enter\n");
	source = attr_search(attrs, NULL, attr_source);
	ret = g_new0(struct vehicle_priv, 1);
	ret->fd = -1;
	ret->cbl = cbl;
	ret->source = g_strdup(source->u.str);
	ret->buffer = g_malloc(buffer_size);
	*meth = vehicle_file_methods;
	if (vehicle_file_open(ret)) {
		vehicle_file_enable_watch(ret);
		return ret;
	}
	dbg(0, "Failed to open '%s'\n", ret->source);
	vehicle_file_destroy(ret);
	return NULL;
}

void
plugin_init(void)
{
	dbg(1, "enter\n");
	plugin_register_vehicle_type("file", vehicle_file_new_file);
	plugin_register_vehicle_type("pipe", vehicle_file_new_file);
}
