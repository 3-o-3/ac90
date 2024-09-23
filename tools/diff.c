/******************************************************************************
 *                       OS-3o3 operating system
 * 
 *                             diff utility
 *
 *            23 August MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/
/*
 * https://github.com/mirror/busybox/blob/master/editors/diff.c
 * https://github.com/paulgb/simplediff/blob/master/php/simplediff.php
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#include <process.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#include <unistd.h>
#else
#include <unistd.h>
#include <wait.h>
#endif

#include "../src/folder.h"

/*
 * types definition
 */

#define _(x) (char*)x

/**
 * load a full file in RAM
 * \param path file name
 * \param size return size of the file
 * \returns an alloced buffer (use free() to release it) containing the contenet of the file
 */
char *diff__load_file(char *path, size_t *size)
{
	FILE *fp;
	char *buf;
	size_t ret;

	fp = fopen((char *)path, "rb");
	if (!fp)
	{
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	*size = (size_t)ftell(fp);
	if (*size < 1)
	{
		return 0;
	}
	fseek(fp, 0, SEEK_SET);
	buf = malloc(*size + 1);
	ret = fread(buf, 1, *size, fp);
	if (ret != *size)
	{
		fprintf(stderr, "ERROR: read fail on %s\n", path);
		return 0;
	}
	fclose(fp);
	buf[*size] = '\0';
	return buf;
}

/**
 * save buffer in file
 * \param path name of the file
 * \param size length of buf
 * \param buf buffer
 * \returns 0 on success
 */
int diff__save_file(char *path, size_t size, char *buf)
{
	FILE *fp;
	size_t ret;
	fp = fopen((char *)path, "wb");
	if (!fp)
	{
		fprintf(stderr, "ERROR: cannot open %s\n", path);
		return -1;
	}

	ret = fwrite(buf, 1, size, fp);
	if (ret != size)
	{
		fprintf(stderr, "ERROR: write fail on %s\n", path);
		return -1;
	}
	fclose(fp);
	return 0;
}

struct array_elem {
	char *txt;
	int len;
	int line;
	char *prefix;
};

struct array_elem *array_elem__new(char * str, int len)
{
	struct array_elem *self;
	self = malloc(sizeof(*self));
	self->txt = str;
	self->len = len;
	self->prefix = _(" ");
	return self;
}


int array_elem__dispose(struct array_elem *self)
{
	free(self);
	return 0;
}

struct array {
	struct array_elem **elem;
	int len;
	int alloced;
};

struct array *array__new()
{
	struct array *self;
	self = malloc(sizeof(*self));
	self->elem = malloc(sizeof(self->elem[0]) * 64);
	self->len = 0;
	self->alloced = 64;
	memset(self->elem, 0, sizeof(self->elem[0]) * 64);
	return self;
}

int array__dispose_children(struct array *self)
{
	while (self->len > 0) {
		self->len--;
		if (self->elem[self->len]) {
			array_elem__dispose(self->elem[self->len]);
		}
	}
	return 0;
}

int array__dispose(struct array *self)
{
	free(self->elem);
	free(self);	
	return 0;
}

int array__set(struct array *self, int at, struct array_elem *el)
{
	int i;

	if (at >= self->alloced) {
		self->elem = realloc(self->elem, 
				sizeof(self->elem[0]) * (at +64));
		for (i = self->alloced; i < self->alloced; i++) {
			self->elem[i] = NULL;
		}
		self->alloced = at + 64;
	}
	self->elem[at] = el;
	if (at >= self->len) {
		self->len = at + 1;
	}	
	return 0;
}

int array__push_line(struct array *self, char *str, int len)
{
	struct array_elem *ae;
	ae = array_elem__new(str, len);
	ae->line = self->len;
	return array__set(self, self->len, ae);
}

struct array *array__consolidate(struct array *o, struct array *n)
{
	struct array *self;
	int  i;
	self = array__new();
	
	i = 0;
	while (i < o->len) {
		o->elem[i]->prefix = _("-");
		array__set(self, self->len, o->elem[i]);
		i++;
	}
	i = 0;
	while (i < n->len) {
		n->elem[i]->prefix = _("+");
		array__set(self, self->len, n->elem[i]);
		i++;
	}
	return self;
}


struct array *array__slice(struct array *self, int offset, int length)
{	struct array *a;
	int i;
	if (length < 0) {
		length = self->len - offset;
	}
	a = array__new();
	i = 0;
	while (i < length) {
		array__set(a, a->len, self->elem[offset]);
		offset++;
		i++;
	}
	return a;
}

struct array *array__merge(struct array *a, struct array *b, struct array *c)
{
	struct array *self;
	int  i;
	self = array__new();
	
	i = 0;
	while (i < a->len) {
		array__set(self, self->len, a->elem[i]);
		i++;
	}
	i = 0;
	while (i < b->len) {
		array__set(self, self->len, b->elem[i]);
		i++;
	}
	i = 0;
	while (i < c->len) {
		array__set(self, self->len, c->elem[i]);
		i++;
	}
	
	return self;
}

struct array *diff__tokenize(char *str, int len)
{
	struct array *a;
	int i;
	int b;
	char c;
	a = array__new();
	i = 0;
	b = 0;
	while (i < len) {
		c = str[i];
		if (c == '\n') {
			array__push_line(a, str + b, i-b);
			b = i + 1;	
		} else if (c == '\r') {
			array__push_line(a, str + b, i-b);
			if (str[i+1] == '\n') {
				i++;
			}	
			b = i + 1;
		}
		i++;
	}
	if (b != i) {
		array__push_line(a, str + b, i-b);
	}
	return a;
}


struct array *diff__process(struct array *o, struct array *n)
{
	struct array *r;
	int *matrix;
	int s;
	int ni;
	int oi;
	int i;
	int p;
	int q;
	int omax;
	int nmax;
	int maxlen;
	struct array_elem *oe;
	struct array_elem *ne;

	s = o->len * n->len;
	matrix = malloc(s * sizeof(*matrix));
	for (i = 0; i < s; i++) {
		matrix[i] = -1;
	}
	omax = -1;
	nmax = -1;
	maxlen = 0;
	for (oi = 0; oi < o->len; oi++) {
		oe = o->elem[oi];
		for (ni = 0; ni < n->len; ni++) {
			ne = n->elem[ni];
			if (!ne || !oe) {
				continue;
			}
			if (oe->txt) {
				if (!ne->txt || ne->len != oe->len) {
					continue;
				}
				for (i = 0; i < oe->len; i++) {
					if (oe->txt[i] != ne->txt[i]) {
						break;;
					}
				}
				if (i != oe->len) {
					continue;
				}
			}
			q = n->len * oi + ni;
			if (oi > 0 && ni > 0) {
				p = n->len * (oi-1) + (ni-1);
			       	if (matrix[p] >= 0) {
					matrix[q] = matrix[p] + 1;
				} else {
					matrix[q] = 1;
				}
			} else {
				matrix[q] = 1;
			}
			if (matrix[q] > maxlen) {
				maxlen = matrix[q];
				omax = oi + 1 - maxlen;
				nmax = ni + 1 - maxlen;
			}
		}
	}
	free(matrix);
	if (maxlen == 0) {
		if (o->len == 0) {
			for (ni = 0; ni < n->len; ni++) {
				n->elem[ni]->prefix = _("+");
			}
		}
		if (n->len == 0) {
			for (oi = 0; oi < o->len; oi++) {
				o->elem[oi]->prefix = _("-");
			}
		}
		r = array__consolidate(o, n);
	} else {
		struct array *a, *b, *c;
		struct array *a1, *a2,  *c1, *c2;
		a1 = array__slice(o, 0, omax);
		a2 = array__slice(n, 0, nmax);
		a = diff__process(a1, a2);
		b = array__slice(n, nmax, maxlen),
		c1 = array__slice(o, omax + maxlen, -1);
		c2 = array__slice(n, nmax + maxlen, -1);
		c = diff__process(c1, c2);
		r = array__merge(a, b, c);
		array__dispose(a1);
		array__dispose(a2);
		array__dispose(a);
		array__dispose(b);
		array__dispose(c1);
		array__dispose(c2);
		array__dispose(c);
	}
	return r;
}

int diff__display(struct array *res, char *o, char *n)
{
	int i, j, k;
	struct array_elem *el;
	int d, ol, nl;
	int l, ll, h;
	int ob, nb;
	int p;

	j = 0;
	h = 0;
	ob = 0;
	nb = 0;
	p = 0;
	for (i = 0; i < res->len; i++) {
		d = -1;
		for (j = i; j < res->len && (j - i) < 4; j++) {
			el = res->elem[j];
			if (el->prefix[0] != ' ') {
				d = j;
				break;
			}
		}
		if (d == -1) {
			ob++;
			nb++;
			continue;
		}
		ol = 0;
		nl = 0;
		l = 0;
		ll = 0;
		if (!p) {
			p = 1;
			printf("diff -uNr %s %s\n", o, n);
		}
		for (k = i; k < res->len && l < 6; k++) {
			el = res->elem[k];
			if (el->prefix[0] == '-') {
				l = 0;
				ll = 0;
			} else if (el->prefix[0] == '+') {
				l = 0;
				ll = 0;
			} else {
				if (ll < 3) {
					ll++;
				}
				l++;
			}
		}
		if (l > ll) {
			k -= ll;
		}
		for (j = i; j < k; j++) {
			el = res->elem[j];
			if (el->prefix[0] == '-') {
				ol++;
			} else if (el->prefix[0] == '+') {
				nl++;
			} else {
				ol++;
				nl++;
			}
		}
		
		if (h == 0) {
			h = 1;
			printf("--- %s\n+++ %s\n", o, n);
		}
		
		printf("@@ -%d,%d +%d,%d @@\n", 
				ob + ((ol>0)? 1 : 0), ol, 
				nb + ((nl>0)? 1 : 0), nl);
		for (; i < j; i++) {	
			el = res->elem[i];
			if (el->prefix[0] == '-') {
				ob++;
			} else if (el->prefix[0] == '+') {
				nb++;
			} else {
				ob++;
				nb++;
			}
			if (el->txt) {
				fwrite(el->prefix, 1, strlen((char*)el->prefix), stdout);
				fwrite(el->txt, 1, el->len, stdout);
				fwrite("\n", 1, 1, stdout);
			}
		}
		i--;
	}
	return 0;
}

int diff__diff(char *old, char *new_)
{
	char *o;
	char *n;
	size_t os;
	size_t ns;
	struct array *oa;
	struct array *na;
	struct array *res;
	o = diff__load_file(old, &os);
	n = diff__load_file(new_, &ns);
	if (o) {
		oa = diff__tokenize(o, os);
	} else {
		oa = array__new();
	}
	if (n) {
		na = diff__tokenize(n, ns);
	} else {
		na = array__new();
	}
	res = diff__process(oa, na);
	diff__display(res, old, new_);
	array__dispose(res);
	array__dispose_children(oa);
	array__dispose_children(na);
	array__dispose(oa);
	array__dispose(na);
	free(o);
	free(n);
	return 0;
}

#define MAX_FILES 0xFFFF

static void diff__expand_folders(const char *name,
	       	char **file_names, int *n)
{
	FOLDER *ffd;
    	char *entry;
    	int l;
    	char fldr[FILENAME_MAX];

    	ffd = openfldr((char*)name);
    	if (!ffd) {
		if (*n < MAX_FILES) {
			file_names[*n] = strdup(name);
			(*n)++;
		}
		return;
    	}
    	if (strcmp(name, _("."))) {
    		snprintf((char*)fldr, sizeof(fldr), "%s", name);
    		l = strlen(fldr);
   		if(fldr[l-1] == '/') {
			l--;
    		}
    	} else {
		l = 0;
		fldr[0] = '\0';
    	}
    	while ((entry = (char*)readfldr(ffd)) != NULL) {
		fldr[l] = '\0';
		if (entry[strlen(entry)-1] == '/') {
	    	if (l) {			
	    		snprintf((char*)fldr +l, sizeof(fldr)-l, "/%s", entry);
	    	} else {
			snprintf((char*)fldr +l, sizeof(fldr)-l, "%s", entry);
	    	}	
	    	diff__expand_folders(fldr, file_names, n);
		} else {
			if (*n < MAX_FILES) {
				if (l) {
					snprintf((char*)fldr +l, sizeof(fldr)-l, "/%s", entry);
				} else {
					snprintf((char*)fldr +l, sizeof(fldr)-l, "%s", entry);
				}
				file_names[*n] = strdup(fldr);
				(*n)++;
			}
		}

    	}
    	closefldr(ffd);
}


/**
 * main entry point
 */
int main(int argc, char *argv[])
{
	char *old;
	char *new_;
	char **ofl;
	char **nfl;
	char *o;
	int onl;
	int nnl;
	int on;
	int nn;
	int i, j;
    	char tmp[FILENAME_MAX];

	if (argc < 3 || argc > 4 || (argc == 4 && argv[1][0] != '-'))
	{
		fprintf(stderr, "usage: %s <old> <new>\n", argv[0]);
		exit(-1);
	}
	if (argc == 3) {
		old = (char*)argv[1];
		new_ = (char*)argv[2];
	} else {
		old = (char*)argv[2];
		new_ = (char*)argv[3];
	}
	ofl = malloc(sizeof(ofl[0]) * MAX_FILES);
	nfl = malloc(sizeof(nfl[0]) * MAX_FILES);
	on = 0;
	nn = 0;
	diff__expand_folders(old, ofl, &on);
	diff__expand_folders(new_, nfl, &nn);
	onl = strlen(old);
	nnl = strlen(new_);
	sprintf((char*)tmp, "%s", old);
	for (i = 0; i < nn; i++) {
		o = NULL;
		for (j = 0; j < on; j++) {
			if (ofl[j] != NULL) {
				if (!strcmp(nfl[i]+nnl, ofl[j]+onl)) {
					o = ofl[j];
					ofl[j] = NULL;
					break;
				}
			}
		}
		if (o) {
			diff__diff(o, nfl[i]);
			free(o);
		} else {
			sprintf((char*)tmp+onl, "%s", nfl[i]+nnl);
			diff__diff(tmp, nfl[i]);
		}
		free(nfl[i]);
	}
	sprintf((char*)tmp, "%s", new_);
	for (j = 0; j < on; j++) {
		if (ofl[j] != NULL) {
			sprintf((char*)tmp+nnl, "%s", ofl[j]+onl);
			diff__diff(ofl[j], tmp);
			free(ofl[j]);
		}
	}
	free(nfl);
	free(ofl);
	return 0;
}
