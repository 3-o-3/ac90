

#include "parser.h"
#include "lexer.h"
#include "token.h"
#include "ast.h"
#include "buf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern int translation_unit(struct parser *self, int mode);

struct parser *parser__new(struct lexer *lex)
{
	struct parser *self;
	self = malloc(sizeof(*self));
	self->lexer = lex;
	self->predict_index = 0;
	self->predict_alloced = 32;
	self->predict = malloc(sizeof(*self->predict) * self->predict_alloced);
	self->ast = ast__new(lex);
	return self;
}

int parser__dispose(struct parser *self)
{
	ast__dispose(self->ast);
	free(self->predict);
	free(self);
	return 0;
}

/*
int parser__print_at(struct parser *self, int at)
{
	struct token *tk;
	int i = 0;
	tk = self->lexer->root;
	while (tk->next && i < at) {
		tk = tk->next;
		if (i >= self->ctx.start) {
			printf(" %s ", lexer__get_value(self->lexer, tk));
		}
		i++;;
	}
	return 0;
}

*/

int parser__error(struct parser *self, const char *txt, int mode)
{
	if (self->status < 0) {
		return -1;
	}
	if (mode != 0) {
		self->status = 1;
		return 0;
	}
	if (self->error_tk == NULL) {
		self->error_tk = self->tk;
		self->error_txt = txt;
	}
	self->status = -1;
	return 0;
}


int parser__push(struct parser *self, int (*func)(struct parser*, int), int mod)
{
	if (self->status) {
		return -1;
	}
	if (self->predict_index >= self->predict_alloced) 
	{
		self->predict_alloced += 32;
		self->predict = realloc(self->predict, 
				sizeof(*self->predict) * self->predict_alloced);

	}
	self->predict[self->predict_index].func = func;
	self->predict[self->predict_index].mode = mod;
	self->predict[self->predict_index].tk = self->tk;
	self->predict_index++;
	return 0;
}

int parser__pop(struct parser *self)
{
	if (self->predict_index > 0) 
	{
		self->predict_index--;
		return 0;
	}
	return -1;
}


int parser__is_loop(struct parser *self)
{
	struct prediction *p;
	p = &self->predict[self->predict_index];
	if (p->tk == self->tk) {
		return 1;
	}
	return 0;
}

int parser__tail(struct parser *self)
{
	struct prediction *p;
	if (self->predict_index <= self->predict_alloced &&
			self->predict_index > 0) 
	{
		self->predict_index--;
		p = &self->predict[self->predict_index];
		return p->func(self, p->mode);
	}
	parser__error(self, "<end>", 1);
	return -1;
}


int parser__parse(struct parser *self)
{
	self->error_tk = NULL;
	self->error_txt = NULL;
	self->status = 0;
	self->tk = self->lexer->root;
	parser__push(self, translation_unit, 0);
	while (self->tk && self->tk->type != token__END_OF_FILE) {
		parser__tail(self);
		if (self->predict_index <= 0) {
			break;
		}
	}
	if (self->tk->type != token__END_OF_FILE) {
		self->error_tk = self->tk;
	}
	if (self->error_tk) {
		printf("\nAt line: (%d) ", lexer__get_line_pos(self->lexer, self->error_tk));
		printf(" %s ", lexer__get_value(self->lexer, self->error_tk));
		printf(" %s ", self->error_txt);
		printf("PARSING FAILED\n");
		return -1;
	} else {
		printf("Parse SUCCESS\n");
	}
	return 0;
}

int parser__eat(struct parser *self)
{
//	printf(" %s ", lexer__get_value(self->lexer, self->tk));
	self->tk = self->tk->next;
	return 0;
}


