#include <stdio.h>
#include "debug.h"
#include "mg.h"

struct tree_hdr_h {
	unsigned int addr;
	unsigned int size;
};

struct tree_leaf_h {
	unsigned int lower;
	unsigned int higher;
	unsigned int match;
	unsigned int value;
};


struct tree_hdr_v {
	unsigned int count;
	unsigned int next;
	unsigned int unknown;
};

struct tree_leaf_v {
	unsigned char key;
	int value;
} __attribute__((packed));

static int
tree_search_h(struct file *file, unsigned int search)
{
	unsigned char *p=file->begin,*end;
	int last,i=0;
	struct tree_hdr_h *thdr;
	struct tree_leaf_h *tleaf;

	dbg(1,"enter\n");
	while (i++ < 1000) {
		thdr=(struct tree_hdr_h *)p;
		p+=sizeof(*thdr);
		end=p+thdr->size;
		dbg(1,"@0x%x\n", p-file->begin);
		last=0;
		while (p < end) {
			tleaf=(struct tree_leaf_h *)p;
			p+=sizeof(*tleaf);
			dbg(1,"low:0x%x high:0x%x match:0x%x val:0x%x search:0x%x\n", tleaf->lower, tleaf->higher, tleaf->match, tleaf->value, search);
			if (tleaf->value == search)
				return tleaf->match;
			if (tleaf->value > search) {
				dbg(1,"lower\n");
				if (tleaf->lower)
					last=tleaf->lower;
				break;
			}
			last=tleaf->higher;
		}
		if (! last || last == -1)
			return 0;
		p=file->begin+last;
	}
	return 0;
}

static int
tree_search_v(struct file *file, int offset, int search)
{
	unsigned char *p=file->begin+offset;
	int i=0,count;
	struct tree_hdr_v *thdr;
	struct tree_leaf_v *tleaf;
	while (i++ < 1000) {
		thdr=(struct tree_hdr_v *)p;
		p+=sizeof(*thdr);
		count=thdr->count;
		dbg(1,"offset=0x%x count=0x%x\n", p-file->begin, count);
		while (count--) {
			tleaf=(struct tree_leaf_v *)p;
			p+=sizeof(*tleaf);
			dbg(1,"0x%x 0x%x\n", tleaf->key, search);
			if (tleaf->key == search)
				return tleaf->value;
		}
		if (! thdr->next)
			break;
		p=file->begin+thdr->next;
	}
	return 0;
}

int
tree_search_hv(char *dirname, char *filename, unsigned int search_h, unsigned int search_v, int *result)
{
	struct file *f_idx_h, *f_idx_v;
	char buffer[4096];
	int h,v;

	dbg(1,"enter(%s, %s, 0x%x, 0x%x, %p)\n",dirname, filename, search_h, search_v, result);
	sprintf(buffer, "%s/%s.h1", dirname, filename);
	f_idx_h=file_create_caseinsensitive(buffer);
	sprintf(buffer, "%s/%s.v1", dirname, filename);
	f_idx_v=file_create_caseinsensitive(buffer);
	dbg(1,"%p %p\n", f_idx_h, f_idx_v);
	if ((h=tree_search_h(f_idx_h, search_h))) {
		dbg(1,"h=0x%x\n", h);
		if ((v=tree_search_v(f_idx_v, h, search_v))) {
			dbg(1,"v=0x%x\n", v);
			*result=v;
			file_destroy(f_idx_v);
			file_destroy(f_idx_h);
			dbg(1,"return 1\n");
			return 1;
		}
	}
	file_destroy(f_idx_v);
	file_destroy(f_idx_h);
	dbg(1,"return 0\n");
	return 0;
}
