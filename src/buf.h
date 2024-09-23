
#ifndef BUF_H_
#define BUF_H_

struct buf {
	char *name;
	char *buf;
	int size;
	int length;
};

struct buf *buf__new(char *name, int size);
int buf__dispose(struct buf *self);
int buf__insert(struct buf *self, char *data, int len, int at) ;
int buf__append_txt(struct buf *self, char *data, int len);
int buf__append_num(struct buf *self, int n);
int buf__clear(struct buf *self);
char *buf__getstr(struct buf *self);
int buf__match_name(struct buf *self, char *txt, int len);
int buf__write(struct buf *self);
int buf__read(struct buf *self);


#endif /* BUF_H_ */
