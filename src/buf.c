
#include "ac90.h"


struct buf *buf__new(char *name, int size)
{
	struct buf *self = malloc(sizeof(struct buf));

	self->name = strdup(name);
	self->size = size;
	self->buf = malloc(self->size);
	self->length = 0;
	self->buf[self->length] = '\0';
	return self;
}

int buf__dispose(struct buf *self)
{
	free(self->name);
	free(self->buf);
	free(self);
	return 0;
}

int buf__insert(struct buf *self, char *data, int dlen, int at) 
{
	int nl;
	int nsize;
	char *nbu;	

	if (dlen < 1) {
		return 0;
	}
	if (at < 0 || at > self->length) {
		at = self->length;
	}
	nl = self->length + dlen;
	if (nl + 1 >= self->size) {
		nsize = self->size + nl + 1;
		nbu = malloc(nsize);
		memcpy(nbu, self->buf, at);
		memcpy(nbu + at, data, dlen);
		memcpy(nbu + at + dlen, self->buf + at, self->length - at);
		nbu[nl] = '\0';
		free(self->buf);
		self->buf = nbu;
		self->length = nl;
		self->size = nsize;
	} else {
		memcpy(self->buf + at + dlen, self->buf + at, self->length - at);
		memcpy(self->buf + at, data, dlen);
		self->buf[nl] = '\0';
		self->length = nl;
	}

	return 0;
}

int buf__append_txt(struct buf *self, char *data, int len) 
{
	if (len < 0) {
		buf__insert(self, data, strlen(data), self->length);
	} else {
		buf__insert(self, data, len, self->length);
	}
	return 0;
}

int buf__append_num(struct buf *self, int n)
{
	char buf[100];
	snprintf((char*)buf, sizeof(buf) -1, "%d", n);
	buf__append_txt(self, buf, -1);
	return 0;
}

int buf__clear(struct buf *self)
{
	free(self->buf);
	self->buf = malloc(32);
	self->buf[0] = '\0';
	self->size = 32;
	self->length = 0;
	return 0;
}

char *buf__getstr(struct buf *self)
{
	return self->buf;
}

int buf__match_name(struct buf *self, char *txt, int len)
{
	if (len < 0)  {
		len = strlen(txt);
	}
	return !strncmp(self->name, txt, len);
}

int buf__write(struct buf *self)
{
	FILE *f;
	int rs;
	
	f = fopen(self->name, "wb+");
	if (!f) {
		fprintf(stderr, "Cannot open %s\n", self->name);
		return -1;
	}
	rs = fwrite(self->buf, 1, self->length, f);
	if (rs != self->length) {
		fprintf(stderr, "Error writing file %s\n", self->name);
		return -1;

	}
	fclose(f);
	return 0;
}
	
int buf__read(struct buf *self)
{
	FILE *f;
	int rs;
	
	f = fopen(self->name, "rb");
	if (!f) {
		fprintf(stderr, "Cannot open %s\n", self->name);
		return -1;
	}
	free(self->buf);
	fseek(f, 0L, SEEK_END);
	self->length = ftell(f);
	fseek(f, 0L, SEEK_SET);

	self->buf = malloc(self->length + 1);
	if (!self->buf) {
		fprintf(stderr, "Cannot allocate %d bytes\n", self->size + 1);
		return -1;
	}
	self->size = self->length + 1;
	rs = fread(self->buf, 1, self->length, f);
	if (rs != self->length) {
		fprintf(stderr, "Error reading file %s\n", self->name);
		return -1;
	}
	self->buf[self->length] = '\0';
	fclose(f);
	return 0;
}


