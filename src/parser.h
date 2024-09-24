
#ifndef PARSER_H_
#define PARSER_H_

struct parser;

struct prediction {
	int mode;
	struct token *tk;
	int (*func)(struct parser*, int);
};

struct parser
{
	struct lexer *lexer;
	struct ast *ast;
	struct token *tk;
	struct prediction *predict;
	int predict_index;
	int predict_alloced;
	struct token *error_tk;
	const char *error_txt;
	int status;
};

struct parser *parser__new(struct lexer *lexer);
int parser__dispose(struct parser *parser);
int parser__parse(struct parser *parser);
int parser__is_loop(struct parser *parser);
int parser__error(struct parser *parser, const char *txt, int mode);
int parser__pop(struct parser *parser);
int parser__push(struct parser *parser, int(*func)(struct parser*,int), int mo);
int parser__eat(struct parser *parser);

#endif /* PARSER_H_ */
