enum attr_type {
	/* common */
	attr_id=1,
	attr_name,
	attr_name_systematic,
	attr_district,
	attr_type,
	/* town */
	
};

struct attr {
	enum attr_type type;
	union {
		char *str;
	} u;
};
