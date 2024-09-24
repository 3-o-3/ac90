
#ifndef PREPROC_H_
#define PREPROC_H_

struct token;

struct preproc
{
	struct buf *tmp;
	int skip;
	struct token *to_expand;
};

struct preproc *preproc__new(void);
int preproc__begin(struct preproc *self, char *file);
int preproc__end(struct preproc *self, char *file);
int preproc__add_line(struct preproc *self, struct token *before);
int preproc__expand(struct preproc *self, struct token *root, 
		struct token **current);
int preproc__dispose(struct preproc *self);

#endif /* PREPROC_H_ */
