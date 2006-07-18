#ifndef ATTR_H
#define ATTR_H

enum attr_type {
	/* common */
	attr_none=0,
	attr_id=1,
	attr_name,
	attr_name_systematic,
	attr_district,
	attr_type,
	/* town */
	attr_size,
	/* poi */
	attr_icon,
	attr_info_html,
	attr_price_html,
};

struct attr {
	enum attr_type type;
	union {
		char *str;
		int num;
	} u;
};

#endif
