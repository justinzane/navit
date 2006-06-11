static inline unsigned char
get_char(unsigned char **p)
{
	return *((*p)++);
}

static inline unsigned short
get_short(unsigned char **p) {
	unsigned short ret;
	ret=*((unsigned short *)*p);
	*p+=sizeof(unsigned short);
	return ret;
}

static inline unsigned short
get_short_unal(unsigned char **p) {
	unsigned short ret;
	ret=*(*p)++;
	ret|=(*(*p)++) << 8;
	return ret;
}


static inline unsigned long
get_triple(unsigned char **p) {
	unsigned long ret;
	ret=get_short(p);
	ret|=*((*p)++) << 16;
	return ret;
}


static inline unsigned long
get_long(unsigned char **p) {
	unsigned long ret;
	ret=*((unsigned int *)*p);
	*p+=sizeof(unsigned int);
	return ret;
}

static inline unsigned long
get_long_unal(unsigned char **p) {
	unsigned long ret;
	ret=*(*p)++;
	ret|=(*(*p)++) << 8;
	ret|=(*(*p)++) << 16;
	ret|=(*(*p)++) << 24;
	return ret;
}

static inline char *
get_string(unsigned char **p)
{
        char *ret=(char *)(*p);
        while (**p) (*p)++;
        (*p)++;
        return ret;
}      

#define L(x) ({ unsigned char *t=(unsigned char *)&(x); t[0] | (t[1] << 8) | (t[2] << 16) | (t[3] << 24); })

