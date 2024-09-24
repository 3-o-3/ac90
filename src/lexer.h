
#ifndef LEXER_H_
#define LEXER_H_

struct lexer
{
	struct buf* buf;
	char *ptr;
	struct token *root;
	struct token *current;
	struct token *preb;
	int line;
	int offset;
	char *file;
	struct buf *tmp;
	struct hash_table *symbols;
	struct preproc *pre;
	int newline;

};

struct lexer *lexer__new(struct preproc *p);
int lexer__dispose(struct lexer *lexer);
int lexer__tokenize(struct lexer *lexer, struct buf *buf, 
		int offset, char *file);
char *lexer__get_value(struct lexer *lexer, struct token *tk);
int lexer__get_line_pos(struct lexer *lexer, struct token *tk);

#endif /* LEXER_H_ */
