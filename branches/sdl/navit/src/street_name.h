#include "types.h"
struct street_name_segment {
	int segid;
	int country;
};

struct street_name {
	u16 len;
	u16 country;
	u32 townassoc;
	char *name1;
	char *name2;
	u32 segment_count;
	struct street_name_segment *segments;
	int aux_len;
	unsigned char *aux_data;
	int tmp_len;
	unsigned char *tmp_data;
};


struct street_name_info {
	u16 len;
	int tag;
	u32 dist;
	u32 country;
	struct coord *c;
	u32 first;
	u32 last;
	u32 segment_count;
	struct street_segment *segments;
	int aux_len;
	unsigned char *aux_data;
	int tmp_len;
	unsigned char *tmp_data;
};

struct street_name_number_info {
	u16 len;
	int tag;
	struct coord *c;
	u32 first;
	u32 last;
	struct street_name_segment *segment;
};

#ifdef __cplusplus
extern "C"
#endif
int street_name_get_by_id(struct street_name *name, struct map_data *mdat, unsigned long id);

void street_name_get(struct street_name *name, unsigned char **p);

#ifdef __cplusplus
extern "C"
#endif
int street_name_search(struct map_data *mdat, int country, int town_assoc, const char *name, int partial, int (*func)(struct street_name *name, void *data), void *data);

#ifdef __cplusplus
extern "C"
#endif
int street_name_get_info(struct street_name_info *inf, struct street_name *name);

#ifdef __cplusplus
extern "C"
#endif
int street_name_get_number_info(struct street_name_number_info *num, struct street_name_info *inf);

