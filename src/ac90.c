
#include "ac90.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
struct pgen
{
	struct buf *in;
	FILE *out;
	int line;
	char *ptr;
	int end_of_rule;
	struct lexer *lexer;
	struct parser *parser;
	struct ast *ast;
};

/****************************************************/

int main(int argc, char *argv[])
{
	struct pgen p;
	if (argc != 3)
	{
		fprintf(stderr, "Usage : %s <source.c> <output.obj>\n", argv[0]);
		exit(-1);
	}
	p.line = 1;
	p.in = buf__new((char *)argv[1], 1024);
	/*p.out = buf__new((char *)argv[2], 1024);*/
	p.out = fopen(argv[2], "wb");
	buf__read(p.in);
	p.lexer = lexer__new();
	if (lexer__tokenize(p.lexer, p.in, 0, p.in->name)) {

		fprintf(stderr, "Error at line(%d) bad token  in file %s\n", 
				p.lexer->line + 1, argv[1]);
		return -1;
	} else {
		fprintf(stderr, "(%d) lines in file \n", p.lexer->line);
	}
	p.parser = parser__new(p.lexer);
	parser__parse(p.parser);
	//p.ast = p.parser->ast;
	
	//ast__gen1(p.ast);

	fclose(p.out);
	/*buf__write(p.out);*/
	parser__dispose(p.parser);
	lexer__dispose(p.lexer);
	buf__dispose(p.in);
	/*buf__dispose(p.out);*/
	return 0;
}
