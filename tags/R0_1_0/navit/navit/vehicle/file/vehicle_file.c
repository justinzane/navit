/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <glib.h>
#include <sys/stat.h>
#ifdef _WIN32
    #include <serial_io.h>
#else
#include <termios.h>
#endif
#include <math.h>
#include "config.h"
#include "debug.h"
#include "callback.h"
#include "plugin.h"
#include "coord.h"
#include "item.h"
#include "event.h"
#include "vehicle.h"

static void vehicle_file_disable_watch(struct vehicle_priv *priv);
static void vehicle_file_enable_watch(struct vehicle_priv *priv);
static int vehicle_file_parse(struct vehicle_priv *priv, char *buffer);
static int vehicle_file_open(struct vehicle_priv *priv);
static void vehicle_file_close(struct vehicle_priv *priv);


enum file_type {
	file_type_pipe = 1, file_type_device, file_type_file
};

static int buffer_size = 1024;

struct vehicle_priv {
	char *source;
	enum file_type file_type;
	struct callback_list *cbl;
	int fd;
	FILE *file;
	struct callback *cb,*cbt;
	struct event_watch *watch;
	char *buffer;
	int buffer_pos;
	char *nmea_data;
	char *nmea_data_buf;

	struct coord_geo geo;
	double speed;
	double direction;
	double height;
	double hdop;
	double vdop;
	char fixtime[20];
	int fixyear;
	int fixmonth;
	int fixday;
	int status;
	int sats_used;
	int sats_visible;
	int time;
	int on_eof;
#ifdef _WIN32
	int no_data_count;
#endif
	speed_t baudrate;
	struct attr ** attrs;
	char fixiso8601[128];
	int checksum_ignore;
};

#ifdef _WIN32
static int vehicle_win32_serial_track(struct vehicle_priv *priv)
{

    static char buffer[2048] = {0,};
    static int current_index = 0;
    const int chunk_size = 1024;
    int rc = 0;
    
    if ( priv->no_data_count > 5 )
    {
        vehicle_file_close( priv );
        priv->no_data_count = 0;
    }

    if ( priv->fd <= 0 )
    {
        vehicle_file_open( priv );
    }

    if ( current_index >= ( sizeof( buffer ) - chunk_size ) )
    {
        // discard
        current_index = 0;
        memset( buffer, 0 , sizeof( buffer ) );
    }

    int dwBytes = serial_io_read( priv->fd, &buffer[ current_index ], chunk_size );
    if ( dwBytes > 0 )
    {
        current_index += dwBytes;

        char* return_pos = NULL;
        while ( ( return_pos = strchr( buffer, '\n' ) ) != NULL )
        {
            char return_buffer[1024];
            int bytes_to_copy = return_pos - buffer + 1;
            memcpy( return_buffer, buffer, bytes_to_copy );
            return_buffer[ bytes_to_copy + 1 ] = '\0';

            // printf( "received %d : '%s' bytes to copy\n", bytes_to_copy, return_buffer );
            rc += vehicle_file_parse( priv, return_buffer );

            current_index -= bytes_to_copy;
            memmove( buffer, &buffer[ bytes_to_copy ] , sizeof( buffer ) - bytes_to_copy );
        }
	if (rc) {
		callback_list_call_0(priv->cbl);
		if (rc > 1)
			dbg(0, "Can not keep with gps data delay is %d seconds\n",
				rc - 1);
	}


    }
    else
    {
        priv->no_data_count++;
    }
    return 1;
}
#endif

static int
vehicle_file_open(struct vehicle_priv *priv)
{
#ifdef _WIN32
    dbg(1, "enter vehicle_file_open, priv->source='%s'\n", priv->source);

    if ( priv->source )
    {
        char* raw_setting_str = g_strdup( priv->source );

        char* strport = strchr(raw_setting_str, ':' );
        char* strsettings = strchr(raw_setting_str, ' ' );

        if ( strport && strsettings )
        {
            strport++;
            *strsettings = '\0';
            strsettings++;

            dbg(1, "calling serial_io_init('%s', '%s')\n", strport, strsettings );
            priv->fd=serial_io_init( strport, strsettings );
        }
        g_free( raw_setting_str );
    }
#else
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
			cfsetispeed(&tio, priv->baudrate);
			cfsetospeed(&tio, priv->baudrate);
			tio.c_cc[VMIN] = 0;
			tio.c_cc[VTIME] = 200;
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
#endif
	return 1;
}

static void
vehicle_file_close(struct vehicle_priv *priv)
{
#ifdef _WIN32
	serial_io_shutdown( priv->fd );
#else
	vehicle_file_disable_watch(priv);
	if (priv->file)
		pclose(priv->file);
	else if (priv->fd >= 0)
		close(priv->fd);
#endif
	priv->file = NULL;
	priv->fd = -1;
}

static int
vehicle_file_enable_watch_timer(struct vehicle_priv *priv)
{
	dbg(1, "enter\n");
	vehicle_file_enable_watch(priv);

	return FALSE;
}


static int
vehicle_file_parse(struct vehicle_priv *priv, char *buffer)
{
	char *nmea_data_buf, *p, *item[16];
	double lat, lng;
	int i, bcsum;
	int len = strlen(buffer);
	unsigned char csum = 0;
	int valid;
	int ret = 0;

	dbg(1, "buffer='%s'\n", buffer);
	for (;;) {
		if (len < 4) {
			dbg(0, "'%s' too short\n", buffer);
			return ret;
		}
		if (buffer[len - 1] == '\r' || buffer[len - 1] == '\n')
			buffer[--len] = '\0';
		else
			break;
	}
	if (buffer[0] != '$') {
		dbg(0, "no leading $ in '%s'\n", buffer);
		return ret;
	}
	if (buffer[len - 3] != '*') {
		dbg(0, "no *XX in '%s'\n", buffer);
		return ret;
	}
	for (i = 1; i < len - 3; i++) {
		csum ^= (unsigned char) (buffer[i]);
	}
	if (!sscanf(buffer + len - 2, "%x", &bcsum) && priv->checksum_ignore != 2) {
		dbg(0, "no checksum in '%s'\n", buffer);
		return ret;
	}
	if (bcsum != csum && priv->checksum_ignore == 0) {
		dbg(0, "wrong checksum in '%s'\n", buffer);
		return ret;
	}

	if (!priv->nmea_data_buf || strlen(priv->nmea_data_buf) < 65536) {
		nmea_data_buf=g_strconcat(priv->nmea_data_buf ? priv->nmea_data_buf : "", buffer, "\n", NULL);
		g_free(priv->nmea_data_buf);
		priv->nmea_data_buf=nmea_data_buf;
	} else {
		dbg(0, "nmea buffer overflow, discarding '%s'\n", buffer);
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
		lat = g_ascii_strtod(item[2], NULL);
		priv->geo.lat = floor(lat / 100);
		lat -= priv->geo.lat * 100;
		priv->geo.lat += lat / 60;

		if (!strcasecmp(item[3],"S"))
			priv->geo.lat=-priv->geo.lat;

		lng = g_ascii_strtod(item[4], NULL);
		priv->geo.lng = floor(lng / 100);
		lng -= priv->geo.lng * 100;
		priv->geo.lng += lng / 60;

		if (!strcasecmp(item[5],"W"))
			priv->geo.lng=-priv->geo.lng;

		sscanf(item[6], "%d", &priv->status);
		sscanf(item[7], "%d", &priv->sats_used);
		sscanf(item[8], "%lf", &priv->hdop);
		strcpy(priv->fixtime, item[1]);
		sscanf(item[9], "%lf", &priv->height);

		g_free(priv->nmea_data);
		priv->nmea_data=priv->nmea_data_buf;
		priv->nmea_data_buf=NULL;
		ret = 1;

#ifndef _WIN32
		if (priv->file_type == file_type_file) {
			if (priv->watch) {
				vehicle_file_disable_watch(priv);
				event_add_timeout(priv->time, 0, priv->cbt);
			}
		}
#endif
	}
	if (!strncmp(buffer, "$GPVTG", 6)) {
		/* 0      1      2 34 5    6 7   8
		   $GPVTG,143.58,T,,M,0.26,N,0.5,K*6A
		   Course Over Ground Degrees True[1],"T"[2],Course Over Ground Degrees Magnetic[3],"M"[4],
		   Speed in Knots[5],"N"[6],"Speed in KM/H"[7],"K"[8]
		 */
		if (item[1] && item[7])
			valid = 1;
		if (i == 9 && (*item[9] == 'A' || *item[9] == 'D'))
			valid = 1;
		if (valid) {
			priv->direction = g_ascii_strtod( item[1], NULL );
			priv->speed = g_ascii_strtod( item[7], NULL );
		}
	}
	if (!strncmp(buffer, "$GPRMC", 6)) {
		/*                                                           1     1
		   0      1      2 3        4 5         6 7     8     9      0     1
		   $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
		   Time[1],Active/Void[2],lat[3],N/S[4],long[5],W/E[6],speed in knots[7],track angle[8],date[9],
		   magnetic variation[10],magnetic variation direction[11]
		 */
		if (*item[2] == 'A')
			valid = 1;
		if (i == 12 && (*item[12] == 'A' || *item[12] == 'D'))
			valid = 1; 
		if (valid) {
			priv->direction = g_ascii_strtod( item[8], NULL );
			priv->speed = g_ascii_strtod( item[7], NULL );
			priv->speed *= 1.852;
			sscanf(item[9], "%02d%02d%02d",
				&priv->fixday,
				&priv->fixmonth,
				&priv->fixyear);
			priv->fixyear += 2000;
		}
	}
	if (!strncmp(buffer, "$GPGSV", 6)) {
	/*
		0 GSV	   Satellites in view
		1 2 	   Number of sentences for full data
		2 1 	   sentence 1 of 2
		3 08	   Number of satellites in view

		4 01	   Satellite PRN number
		5 40	   Elevation, degrees
		6 083	   Azimuth, degrees
		7 46	   SNR - higher is better
			   for up to 4 satellites per sentence
		*75	   the checksum data, always begins with *
	*/
		if (item[3]) {
			sscanf(item[3], "%d", &priv->sats_visible);
		}
	}
	if (!strncmp(buffer, "$GPZDA", 6)) {
	/*
		0        1        2  3  4    5  6
		$GPZDA,hhmmss.ss,dd,mm,yyyy,xx,yy*CC
			hhmmss    HrMinSec(UTC)
			dd,mm,yyy Day,Month,Year
			xx        local zone hours -13..13
			yy        local zone minutes 0..59
	*/
		if (item[1] && item[2] && item[3] && item[4]) {
			// priv->fixtime = atof(item[1]);
			strcpy(priv->fixtime, item[1]);
			priv->fixday = atoi(item[2]);
			priv->fixmonth = atoi(item[3]);
			priv->fixyear = atoi(item[4]);
		}
	}
	return ret;
}

#ifndef _WIN32
static void
vehicle_file_io(struct vehicle_priv *priv)
{
	int size, rc = 0;
	char *str, *tok;

	dbg(1, "enter\n");
	size = read(priv->fd, priv->buffer + priv->buffer_pos, buffer_size - priv->buffer_pos - 1);
	if (size <= 0) {
		switch (priv->on_eof) {
		case 0:
			vehicle_file_close(priv);
			vehicle_file_open(priv);
			break;
		case 1:
			vehicle_file_disable_watch(priv);
			break;
		case 2:
			exit(0);
			break;
		}
		return;
	}
	priv->buffer_pos += size;
	priv->buffer[priv->buffer_pos] = '\0';
	dbg(1, "size=%d pos=%d buffer='%s'\n", size,
	    priv->buffer_pos, priv->buffer);
	str = priv->buffer;
	while ((tok = strchr(str, '\n'))) {
		*tok++ = '\0';
		dbg(1, "line='%s'\n", str);
		rc +=vehicle_file_parse(priv, str);
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
	if (rc)
		callback_list_call_0(priv->cbl);
}
#endif

static void
vehicle_file_enable_watch(struct vehicle_priv *priv)
{
#ifdef _WIN32
	g_timeout_add(500, vehicle_win32_serial_track, priv);
#else
	if (! priv->watch)
		priv->watch = event_add_watch((void *)priv->fd, event_watch_cond_read, priv->cb);
#endif
}

static void
vehicle_file_disable_watch(struct vehicle_priv *priv)
{
#ifndef _WIN32
	if (priv->watch)
		event_remove_watch(priv->watch);
	priv->watch = NULL;
#endif
}


static void
vehicle_file_destroy(struct vehicle_priv *priv)
{
	vehicle_file_close(priv);
	callback_destroy(priv->cb);
	callback_destroy(priv->cbt);
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
	struct attr * active=NULL;

	switch (type) {
	case attr_position_fix_type:
		attr->u.num = priv->status;
		break;
	case attr_position_height:
		attr->u.numd = &priv->height;
		break;
	case attr_position_speed:
		attr->u.numd = &priv->speed;
		break;
	case attr_position_direction:
		attr->u.numd = &priv->direction;
		break;
	case attr_position_hdop:
		attr->u.numd = &priv->hdop;
		break;
	case attr_position_qual:
		attr->u.num = priv->sats_visible;
		break;
	case attr_position_sats_used:
		attr->u.num = priv->sats_used;
		break;
	case attr_position_coord_geo:
		attr->u.coord_geo = &priv->geo;
		break;
	case attr_position_nmea:
		attr->u.str=priv->nmea_data;
		if (! attr->u.str)
			return 0;
		break;
	case attr_position_time_iso8601:
		if (!priv->fixyear || !priv->fixtime[0])
			return 0;
		sprintf(priv->fixiso8601, "%04d-%02d-%02dT%sZ",
			priv->fixyear, priv->fixmonth, priv->fixday,
			priv->fixtime);
		attr->u.str=priv->fixiso8601;
		break;
	case attr_active:
		  if(active != NULL && active->u.num == 1)
		    return 1;
		  else
		    return 0;
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
	struct attr *time;
	struct attr *on_eof;
	struct attr *baudrate;
	struct attr *checksum_ignore;

	dbg(1, "enter\n");
	source = attr_search(attrs, NULL, attr_source);
	ret = g_new0(struct vehicle_priv, 1);
	ret->fd = -1;
	ret->cbl = cbl;
	ret->source = g_strdup(source->u.str);
	ret->buffer = g_malloc(buffer_size);
	ret->time=1000;
	ret->baudrate=B4800;
	time = attr_search(attrs, NULL, attr_time);
	if (time)
		ret->time=time->u.num;
	baudrate = attr_search(attrs, NULL, attr_baudrate);
	if (baudrate) {
		switch (baudrate->u.num) {
		case 4800:
			ret->baudrate=B4800;
			break;
		case 9600:
			ret->baudrate=B9600;
			break;
		case 19200:
			ret->baudrate=B19200;
			break;
#ifdef B38400
		case 38400:
			ret->baudrate=B38400;
			break;
#endif
#ifdef B57600
		case 57600:
			ret->baudrate=B57600;
			break;
#endif
#ifdef B115200
		case 115200:
			ret->baudrate=B115200;
			break;
#endif
		}
	}
	checksum_ignore = attr_search(attrs, NULL, attr_checksum_ignore);
	if (checksum_ignore)
		ret->checksum_ignore=checksum_ignore->u.num;
	ret->attrs = attrs;
	on_eof = attr_search(attrs, NULL, attr_on_eof);
	if (on_eof && !strcasecmp(on_eof->u.str, "stop"))
		ret->on_eof=1;
	if (on_eof && !strcasecmp(on_eof->u.str, "exit"))
		ret->on_eof=2;
	dbg(0,"on_eof=%d\n", ret->on_eof);
	*meth = vehicle_file_methods;
	ret->cb=callback_new_1(callback_cast(vehicle_file_io), ret);
	ret->cbt=callback_new_1(callback_cast(vehicle_file_enable_watch_timer), ret);
	if (vehicle_file_open(ret)) {
		vehicle_file_enable_watch(ret);
		return ret;
	}

#ifdef _WIN32
	ret->no_data_count = 0;
#endif
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
