#ifndef COORD_H
#define COORD_H

/*! A integer mercator coordinate */
struct coord {
	int x; /*!< X-Value */
	int y; /*!< Y-Value */
};

struct coord_rect {
	struct coord lu;
	struct coord rl;
};

//! A double mercator coordinate
struct coord_d {
	double x; /*!< X-Value */
	double y; /*!< Y-Value */
};

//! A WGS84 coordinate
struct coord_geo {
	double lng; /*!< Longitude */
	double lat; /*!< Latitude */
};

struct coord * coord_get(unsigned char **p);
int coord_rect_overlap(struct coord_rect *r1, struct coord_rect *r2);
int coord_rect_contains(struct coord_rect *r, struct coord *c);


#endif
