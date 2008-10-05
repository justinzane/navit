#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <windows.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#if 0
#include <sys/mman.h>
#endif
#include <dirent.h>
#include <stdio.h>
#if 0
#include <wordexp.h>
#endif
#include <glib.h>
#include <zlib.h>
#include "debug.h"
#include "file.h"
#include "config.h"

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

static struct file *file_list;

struct file *
file_create(char *name)
{
	struct stat stat;
	struct file *file= g_new0(struct file,1);

	if (! file)
		return file;
	file->fd=open(name, O_RDONLY|O_LARGEFILE | O_BINARY);
	if (file->fd < 0) {
		g_free(file);
		return NULL;
	}
	fstat(file->fd, &stat);
	file->size=stat.st_size;
	file->name = g_strdup(name);
	g_assert(file != NULL);
	file->next=file_list;
	file_list=file;
	return file;
}

int file_is_dir(char *name)
{
	struct stat buf;
	if (! stat(name, &buf)) {
		return S_ISDIR(buf.st_mode);
	}
	return 0;

}

int file_mkdir(char *name, int pflag)
{
	char buffer[strlen(name)+1];
	int ret;
	char *curr, *next;
	dbg(1,"enter %s %d\n",name,pflag);
	if (!pflag) {
		if (file_is_dir(name))
			return 0;
#if !defined(__CEGCC__) && defined(_WIN32)
		return mkdir(name);
#else
		return mkdir(name, 0777);
#endif
	}
	strcpy(buffer, name);
	next=buffer;
	while (next=strchr(next, '/')) {
		*next='\0';
		ret=file_mkdir(buffer, 0);
		if (ret)
			return ret;
		*next++='/';
	}
	if (pflag == 1)
#if !defined(__CEGCC__) && defined(_WIN32)
		return mkdir(name);
#else
		return mkdir(name, 0777);
#endif
	return 0;
}

int
file_mmap(struct file *file)
{
#if 0
#if defined(_WIN32) || defined(__CEGCC__)
    file->begin = (unsigned char*)mmap_readonly_win32( file->name, &file->map_handle, &file->map_file );
#else
	file->begin=mmap(NULL, file->size, PROT_READ|PROT_WRITE, MAP_PRIVATE, file->fd, 0);
	g_assert(file->begin != NULL);
	if (file->begin == (void *)0xffffffff) {
		perror("mmap");
		return 0;
	}
#endif
	g_assert(file->begin != (void *)0xffffffff);
	file->end=file->begin+file->size;

	return 1;
#endif
	return 0;
}

unsigned char *
file_data_read(struct file *file, long long offset, int size)
{
	void *ret;
	if (file->begin)
		return file->begin+offset;
	ret=g_malloc(size);
	lseek(file->fd, offset, SEEK_SET);
	if (read(file->fd, ret, size) != size) {
		g_free(ret);
		ret=NULL;
	}
	return ret;

}

static int
uncompress_int(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
	z_stream stream;
	int err;

	stream.next_in = (Bytef*)source;
	stream.avail_in = (uInt)sourceLen;
	stream.next_out = dest;
	stream.avail_out = (uInt)*destLen;

	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;

	err = inflateInit2(&stream, -MAX_WBITS);
	if (err != Z_OK) return err;

	err = inflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
        inflateEnd(&stream);
        if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
            return Z_DATA_ERROR;
		return err;
	}
	*destLen = stream.total_out;

	err = inflateEnd(&stream);
	return err;
}

unsigned char *
file_data_read_compressed(struct file *file, long long offset, int size, int size_uncomp)
{
	void *ret;
	char buffer[size];
	uLongf destLen=size_uncomp;

	ret=g_malloc(size_uncomp);
	lseek(file->fd, offset, SEEK_SET);
	if (read(file->fd, buffer, size) != size) {
		g_free(ret);
		ret=NULL;
	} else {
		if (uncompress_int(ret, &destLen, (Bytef *)buffer, size) != Z_OK) {
			dbg(0,"uncompress failed\n");
			g_free(ret);
			ret=NULL;
		}
	}
	return ret;
}

void
file_data_free(struct file *file, unsigned char *data)
{
	if (file->begin && data >= file->begin && data < file->end)
		return;
	g_free(data);
}

int
file_exists(char *name)
{
	struct stat buf;
	if (! stat(name, &buf))
		return 1;
	return 0;
}

void
file_remap_readonly(struct file *f)
{
#if defined(_WIN32) || defined(__CEGCC__)
#else
	void *begin;
	munmap(f->begin, f->size);
	begin=mmap(f->begin, f->size, PROT_READ, MAP_PRIVATE, f->fd, 0);
	if (f->begin != begin)
		printf("remap failed\n");
#endif
}

void
file_remap_readonly_all(void)
{
	struct file *f=file_list;
	int limit=1000;

	while (f && limit-- > 0) {
		file_remap_readonly(f);
		f=f->next;
	}
}

void
file_unmap(struct file *f)
{
#if 0
#if defined(_WIN32) || defined(__CEGCC__)
    mmap_unmap_win32( f->begin, f->map_handle , f->map_file );
#else
	munmap(f->begin, f->size);
#endif
#endif
}

void
file_unmap_all(void)
{
	struct file *f=file_list;
	int limit=1000;

	while (f && limit-- > 0) {
		file_unmap(f);
		f=f->next;
	}
}



void *
file_opendir(char *dir)
{
	return opendir(dir);
}

char *
file_readdir(void *hnd)
{
	struct dirent *ent;

	ent=readdir(hnd);
	if (! ent)
		return NULL;
	return ent->d_name;
}

void
file_closedir(void *hnd)
{
	closedir(hnd);
}

struct file *
file_create_caseinsensitive(char *name)
{
	char dirname[strlen(name)+1];
	char *filename;
	char *p;
	void *d;
	struct file *ret;

	ret=file_create(name);
	if (ret)
		return ret;

	strcpy(dirname, name);
	p=dirname+strlen(name);
	while (p > dirname) {
		if (*p == '/')
			break;
		p--;
	}
	*p=0;
	d=file_opendir(dirname);
	if (d) {
		*p++='/';
		while ((filename=file_readdir(d))) {
			if (!strcasecmp(filename, p)) {
				strcpy(p, filename);
				ret=file_create(dirname);
				if (ret)
					break;
			}
		}
		file_closedir(d);
	}
	return ret;
}

void
file_destroy(struct file *f)
{
	close(f->fd);

    if ( f->begin != NULL )
    {
        file_unmap( f );
    }

	g_free(f->name);
	g_free(f);
}

#ifndef _WORDEXP_H_
#define	_WORDEXP_H_


typedef struct {
	size_t	we_wordc;		/* count of words matched */
	char		**we_wordv;	/* pointer to list of words */
	size_t	we_offs;		/* slots to reserve in we_wordv */
					/* following are internals */
	char		*we_strings;	/* storage for wordv strings */
	size_t	we_nbytes;		/* size of we_strings */
} wordexp_t;

/*
 * Flags for wordexp().
 */
#define	WRDE_APPEND	0x1		/* append to previously generated */
#define	WRDE_DOOFFS	0x2		/* we_offs member is valid */
#define	WRDE_NOCMD	0x4		/* disallow command substitution */
#define	WRDE_REUSE	0x8		/* reuse wordexp_t */
#define	WRDE_SHOWERR	0x10		/* don't redirect stderr to /dev/null */
#define	WRDE_UNDEF	0x20		/* disallow undefined shell vars */

/*
 * Return values from wordexp().
 */
#define	WRDE_BADCHAR	1		/* unquoted special character */
#define	WRDE_BADVAL	2		/* undefined variable */
#define	WRDE_CMDSUB	3		/* command substitution not allowed */
#define	WRDE_NOSPACE	4		/* no memory for result */
#if (_XOPEN_SOURCE - 0) >= 4 || defined(_NETBSD_SOURCE)
#define	WRDE_NOSYS	5		/* obsolete, reserved */
#endif
#define	WRDE_SYNTAX	6		/* shell syntax error */
#define WRDE_ERRNO	7		/* other errors see errno */

void	wordfree(wordexp_t *);
int wordexp(const char * words, wordexp_t * we, int flags);


#endif /* !_WORDEXP_H_ */

#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int wordexp(const char * words, wordexp_t * we, int flags)
{
	int error=0;

	assert(we != NULL);
	assert(words != NULL);

    we->we_wordc = 1;
	we->we_wordv = NULL;
	we->we_strings = NULL;
	we->we_nbytes = 0;

    we->we_wordv = malloc( we->we_wordc * sizeof( char* ) );

    we->we_nbytes = strlen( words ) + 1;
    we->we_strings = malloc( we->we_nbytes );

    we->we_wordv[0] = &we->we_strings[0];

    // copy string & terminate
    memcpy( we->we_strings, words, we->we_nbytes -1 );
    we->we_strings[ we->we_nbytes -1 ] = '\0';

	return error;
}

void wordfree(wordexp_t *we)
{
	assert(we != NULL);

	if ( we->we_wordv )
	{
        free(we->we_wordv);
	}
	if ( we->we_strings )
	{
        free(we->we_strings);
	}

	we->we_wordv = NULL;
	we->we_strings = NULL;
	we->we_nbytes = 0;
	we->we_wordc = 0;
}
struct file_wordexp {
    int err;
	wordexp_t we;
};

struct file_wordexp *
file_wordexp_new(const char *pattern)
{
	struct file_wordexp *ret=g_new0(struct file_wordexp, 1);

	ret->err=wordexp(pattern, &ret->we, 0);
	if (ret->err)
		dbg(0,"wordexp('%s') returned %d\n", pattern, ret->err);
	return ret;
	return NULL;
}

int
file_wordexp_get_count(struct file_wordexp *wexp)
{
	return wexp->we.we_wordc;
}

char **
file_wordexp_get_array(struct file_wordexp *wexp)
{
	return wexp->we.we_wordv;
}

void
file_wordexp_destroy(struct file_wordexp *wexp)
{
	if (! wexp->err)
		wordfree(&wexp->we);
	g_free(wexp);
}


int
file_get_param(struct file *file, struct param_list *param, int count)
{
	int i=count;
	param_add_string("Filename", file->name, &param, &count);
	param_add_hex("Size", file->size, &param, &count);
	return i-count;
}
