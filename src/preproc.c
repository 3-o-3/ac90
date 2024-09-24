
#include "buf.h"
#include "preproc.h"
#include "lexer.h"
#include "token.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct preproc *preproc__new(void)
{
	struct preproc *self;
	self = malloc(sizeof(*self));
	self->tmp = buf__new("", 1024);
	self->skip = 0;
	self->to_expand = NULL;
	return self;
}

int preproc__dispose(struct preproc *self)
{
	buf__dispose(self->tmp);
	free(self);
	return 0;
}

int preproc__begin(struct preproc *self, char *file)
{
	return 0;
}

int preproc__end(struct preproc *self, char *file)
{
	return 0;
}

int preproc__add_line(struct preproc *self, struct token *before)
{
	struct token *t;
	struct token *nxt;
	if (!before) {
		return -1;
	}
	t = before->next;
	if (t->type != token__HASHTAG) {
		return -1;
	}
	if (!t->next) {
		before->next = NULL;
		token__dispose(t);
		return 0;
	}
	t = t->next;
	switch (t->type) {
	case token__IF:
	case token__ELSE:
	case token__DEFINED:
	case token__ELIF:
	case token__ERROR:
	case token__IFDEF:
	case token__IFNDEF:
	case token__LINE:
	case token__PRAGMA:
	case token__UNDEF:
	case token__INCLUDE:
	case token__ENDIF:
	case token__IDENTIFIER:
		break;
	case token__DEFINE_FUNC:
	case token__DEFINE:
		break;
	}
			
	while (t) {
		nxt = t->next;
		token__dispose(t);
		t = nxt;
	}
	before->next = NULL;
	return 0;
}

int preproc__expand(struct preproc *self, struct token *root, 
		struct token **current)
{
	struct token *t;
	struct token *nxt;
	if (self->to_expand == NULL) {
		self->to_expand = root;
	}
	t = self->to_expand;
	while (t->next) {
		nxt = t->next;
		switch (t->type) {
		case token__DEFINED:
		case token__ELIF:
		case token__ERROR:
		case token__IFDEF:
		case token__IFNDEF:
		case token__LINE:
		case token__PRAGMA:
		case token__UNDEF:
		case token__DEFINE:
		case token__INCLUDE:
		case token__DEFINE_FUNC:
		case token__ENDIF:
			t->type = token__IDENTIFIER;
		case token__IDENTIFIER:
			// FIXME expand
			break;
		}
		t = nxt;
	}
	t = self->to_expand;
	while (t->next) {
		nxt = t->next;
		if (t->type == token__STRING_LITERAL) {
			// FIXME must merge strings
		}
		t = nxt;
	}
	

	self->to_expand = t;
	return 0;
}

