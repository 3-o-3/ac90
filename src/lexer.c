/******************************************************************************

              ac90: public domain C compiler

           MMXXIV September 23 PUBLIC DOMAIN by JML

     The authors and contributors disclaim copyright, patents 
           and all related rights to this software.

 Anyone is free to copy, modify, publish, use, compile, sell, or
 distribute this software, either in source code form or as a
 compiled binary, for any purpose, commercial or non-commercial,
 and by any means.

 The authors waive all rights to patents, both currently owned 
 by the authors or acquired in the future, that are necessarily 
 infringed by this software, relating to make, have made, repair,
 use, sell, import, transfer, distribute or configure hardware 
 or software in finished or intermediate form, whether by run, 
 manufacture, assembly, testing, compiling, processing, loading 
 or applying this software or otherwise.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT OF ANY PATENT, COPYRIGHT, TRADE SECRET OR OTHER
 PROPRIETARY RIGHT.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR 
 ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
 CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************/

#include "lexer.h"
#include "token.h"
#include "buf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct lexer *lexer__new(void)
{
	struct lexer *self;
	self = malloc(sizeof(*self));
	self->buf = NULL;
	self->ptr = NULL;
	self->file = "";
	self->line = 0;
	self->offset = 0;
	self->root = token__new(token__ROOT, self->ptr);
	self->current = self->root;
	self->tmp = buf__new("tmp", 80);
	return self;
}

int lexer__dispose(struct lexer *self)
{
	struct token *t, *n;
	t = self->root;
	while (t) {
		n = t->next;
		token__dispose(t);
		t = n;
	}
	free(self);
	return 0;
}

int lexer__warning(struct lexer *self, char *opt, char *txt)
{
	fprintf(stderr, "%s:%d: warning: %s\n", 
			self->file, self->line + 1, txt);
	return 0;
}

int lexer__error(struct lexer *self, char *txt)
{
	fprintf(stderr, "%s:%d: error: %s\n", 
			self->file, self->line + 1, txt);
	exit(-1);
	return 0;
}


/*
 * A12.1 Trigraph sequences
 */
int lexer__trigraphs(struct lexer *self)
{
	char *p;
	int t;
	char *c;

	p = self->ptr;
	c = p;
	while (*p && *p != '\n') {
		t = 0;
		if (p[0] == '?' && p[1] == '?') {
			switch (p[2]) {
			case '=':
				t = '#';
				break;
			case '/':
				t = '\\';
				break;
			case '\'':
				t = '^';
				break;
			case '(':
				t = '[';
				break;
			case ')':
				t = ']';
				break;
			case '!':
				t = '|';
				break;
			case '<':
				t = '{';
				break;
			case '>':
				t = '}';
				break;
			case '-':
				t = '~';
				break;
			}
		}
		if (t) {
			p += 3;
			if (t == '\\') {
				/* A12.2 Line splicing */
				if (p[0] == '\n') {
					p++;
					self->line++;
				} else if (p[0] == '\r' && p[1] == '\n') {
					p += 2;
					self->line++;
				} else {
					*c = t;
					c++;
				}
			} else {
				*c = t;
				c++;
			}
		} else {
			if (p[0] == '\\') {
				/* A12.2 Line splicing */
				if (p[1] == '\n') {
					p += 2;
					self->line++;
				} else if (p[1] == '\r' && p[2] == '\n') {
					p += 3;
					self->line++;
				} else {
					*c = *p;
					c++;
					p++;
				}
			} else {
				*c = *p;
				c++;
				p++;
			}
		}
	}
	while (c != p) {
		*c = ' ';
		c++;
	}
	return 0;
}

/* A2.1 Tokens */
int lexer__whitespace(struct lexer *self)
{
	char *p;

	p = self->ptr;
	while (*p == ' ' || *p == '\t' || *p == '\r' || 
			*p == '\n' || *p == '\v')
	{
		if (*p == '\n')
		{
			self->ptr = p + 1;
			lexer__trigraphs(self);
			self->line++;
		}
		p++;
	}
	if (p[0] == '/' && p[1] == '/')
	{
		self->ptr = p;
		lexer__warning(self, "-Wcomment", "\"//\" comment not allowed");

		while (*p && *p != '\n')
		{
			p++;
		}
		self->ptr = p;
		lexer__whitespace(self);
		return 1;
	} else if (p[0] == '/' && p[1] == '*') {
		/* A2.2 Comments */
		while (*p && (p[0] != '*' || p[1] != '/'))
		{
			if (*p == '\n') {
				self->line++;
			}
			p++;
		}
		if (!*p) {
			self->ptr = p;
			return 0;
		}
		self->ptr = p + 2;
		lexer__whitespace(self);
		return 1;
	}
	if (p != self->ptr) {
		self->ptr = p;
	}
	return 0;
}

int lexer__constant(struct lexer *self)
{
	char *p;
	char *b;
	p = self->ptr;
	if (*p != '"')
	{
		return -1;
	}
	b = p;
	p++;
	while (*p)
	{
		if (*p == '"')
		{
			break;
		}
		if (*p == '\\')
		{
			p++;
		}
		p++;
	}
	if (p[0] == '"')
	{
		if (p[1] == '=')
		{
			self->current->next = token__new(token__END_OF_RULE, b);
			self->current = self->current->next;
		}
		self->current->next = token__new(token__CONSTANT, b);
		self->current = self->current->next;
		p++;
		self->ptr = p;
		return 0;
	}
	return -1;
}

int lexer__identifier(struct lexer *self, char *b, char *p)
{
	int t = token__IDENTIFIER;
	int l = (int)(p - b);

	switch (*b) {
	case 'a':
		if (l == 4) {
			if (!strncmp("auto", b, 4)) {
				t = token__AUTO;
			}
		}
		break;	
	case 'b':
		if (l == 5) {
			if (!strncmp("break", b, 5)) {
				t = token__BREAK;
			}
		}
		break;	
	case 'c':
		if (l == 4) {
			if (b[1] == 'a' && !strncmp("case", b, 4)) {
				t = token__CASE;
			} else if (b[1] == 'h' && !strncmp("char", b, 4)) {
				t = token__CHAR;
			}
		} else if (l == 5) {
			if (!strncmp("const", b, 5)) {
				t = token__CONST;
			}
		} else if (l == 8) {
			if (!strncmp("continue", b, 8)) {
				t = token__CONTINUE;
			}
		}
		break;	
	case 'd':
		if (l == 7) {
			if (!strncmp("default", b, 7)) {
				t = token__DEFAULT;
			}
		} else if (l == 2) {
			if (b[1] == 'o') {
				t = token__DO;
			}
		} else if (l == 6) {
			if (b[1] == 'o' && !strncmp("double", b, 6)) {
				t = token__DOUBLE;
			} else if (b[1] == 'e' && !strncmp("define", b, 6)) {
				t = token__DEFINE;
			}
		} else if (l == 7) {
			if (!strncmp("defined", b, 7)) {
				t = token__DEFINED;
			}
		}
		break;	
	case 'e':
		if (l == 4) {
			if (b[2] == 's' && !strncmp("else", b, 4)) {
				t = token__ELSE;
			} else if (b[2] == 'i' && !strncmp("elif", b, 4)) {
				t = token__ELIF;
			} else if (b[1] == 'n' && !strncmp("enum", b, 4)) {
				t = token__ENUM;
			}
		} else if (l == 6) {
			if (!strncmp("extern", b, 6)) {
				t = token__EXTERN;
			}
		} else if (l == 5) {
			if (!strncmp("error", b, 5)) {
				t = token__ERROR;
			}
		}
		break;
	case 'f':
		if (l == 5) {
			if (!strncmp("float", b, 5)) {
				t = token__FLOAT;
			}
		} else if (l == 3) {
			if (b[1] == 'o' && b[2] == 'r') {
				t = token__FOR;
			}
		}
		break;
	case 'g':
		if (l == 4) {
			if (!strncmp("goto", b, 4)) {
				t = token__GOTO;
			}
		}
		break;
	case 'i':
		if (l == 3) {
			if (!strncmp("int", b, 3)) {
				t = token__INT;
			}
		} else if (l == 2) {
			if (b[1] == 'f') {
				t = token__IF;
			}
		} else if (l == 5) {
			if (!strncmp("ifdef", b, 5)) {
				t = token__IFDEF;
			}
		} else if (l == 6) {
			if (!strncmp("ifndef", b, 6)) {
				t = token__IFNDEF;
			}
		} else if (l == 7) {
			if (!strncmp("include", b, 7)) {
				t = token__INCLUDE;
			}
		}
		break;
	case 'l':
		if (l == 4) {
			if (b[1] == 'o' && !strncmp("long", b, 4)) {
				t = token__LONG;
			} else if (b[1] == 'i' && !strncmp("line", b, 4)) {
				t = token__LINE;
			}
		}
		break;
	case 'p':
		if (l == 6) {
			if (!strncmp("pragma", b, 6)) {
				t = token__PRAGMA;
			}
		}
		break;
	case 'r':
		if (l == 8) {
			if (!strncmp("register", b, 8)) {
				t = token__REGISTER;
			}
		} else if (l == 6) {
			if (!strncmp("return", b, 6)) {
				t = token__RETURN;
			}
		}
		break;
	case 's':
		if (l == 5) {
			if (!strncmp("short", b, 5)) {
				t = token__SHORT;
			}
		} else if (l == 6) {
			if (b[1] == 'i') {
				if (b[2] == 'g' && !strncmp("signed", b, 6)) {
					t = token__SIGNED;
				} else if (b[2] == 'z' &&
						!strncmp("sizeof", b, 6)) 
				{
					t = token__SIZEOF;
				}
			} else if (b[1] == 't') {
				if (b[2] == 'a' && !strncmp("static", b, 6)) {
					t = token__STATIC;
				} else if (b[2] == 'r' && 
						!strncmp("struct", b, 6)) 
				{
					t = token__STRUCT;
				}
			} else if (b[1] == 'w') {
				if (!strncmp("switch", b, 6)) {
					t = token__SWITCH;
				}
			}

		}
		break;
	case 't':
		if (l == 7) {
			if (!strncmp("typedef", b, 7)) {
				t = token__TYPEDEF;
			}
		}
		break;
	case 'u':
		if (l == 5) {
			if (!strncmp("union", b, 5)) {
				t = token__UNION;
			} else if (!strncmp("undef", b, 5)) {
				t = token__UNDEF;
			}
		} else if (l == 8) {
			if (!strncmp("unsigned", b, 8)) {
				t = token__UNSIGNED;
			}
		}
		break;
	case 'v':
		if (l == 4) {
			if (!strncmp("void", b, 4)) {
				t = token__VOID;
			}
		} else if (l == 8) {
			if (!strncmp("volatile", b, 8)) {
				t = token__VOLATILE;
			}
		}
		break;
	case 'w':
		if (l == 5) {
			if (!strncmp("while", b, 5)) {
				t = token__WHILE;
			}
		}
		break;
	
	}
	self->current->next = token__new(t, b);
	self->current = self->current->next;
	self->ptr = p;
	return 0;
}

int lexer__string_literal(struct lexer *self)
{
	return 0;
}

int lexer__character_constant(struct lexer *self)
{
	return 0;
}

int lexer__next(struct lexer *self)
{
	char *p;
	char *b;
	int n;
	int t = 0;

	lexer__whitespace(self);

	p = self->ptr;
	b = p;
	if (*p == '_' || (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z'))
	{
		n = 1;
		p++;
		while (*p == '_' || (*p >= 'a' && *p <= 'z') ||
			   (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))
		{
			n++;
			p++;
		}
		if (n > 31) {
			lexer__warning(self, "-Widentifier-length", 
			   "only 31 characters are significant in identifiers");
		}
		return lexer__identifier(self, b, p);
	} 
	switch (*p) {
	case '"':
		return lexer__string_literal(self);
	case '\'':
		return lexer__character_constant(self);
	case '.':
		if (p[1] >= '0' && p[1] <= '9') {
			return lexer__constant(self);
		} else if (p[1] == '.' && p[2] == '.') {
			t = token__ELLIPSIS;
			p += 2;
		} else {
			t = token__DOT;
		}
		p++;
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return lexer__constant(self);
	case '!':
		if (p[1] == '=') {
			t = token__NOTEQ;
			p++;
		} else {
			t = token__XMARK;
		}
		p++;
		break;
	case '%':
		if (p[1] == '=') {
			t = token__ASMOD;
			p++;
		} else {
			t = token__MOD;
		}
		p++;
		break;
	case '/':
		if (p[1] == '=') {
			t = token__ASDIV;
			p++;
		} else {
			t = token__DIV;
		}
		p++;
		break;
	case '*':
		if (p[1] == '=') {
			t = token__ASMUL;
			p++;
		} else {
			t = token__MUL;
		}
		p++;
		break;
	case '?':
		t = token__QMARK;
		p++;
		break;
	case '|':
		if (p[1] == '|') {
			t = token__LOGOR;
			p++;
		} else if (p[1] == '=') {
			t = token__ASOR;
			p++;
		} else {
			t = token__PIPE;
		}
		p++;
		break;
	case '&':
		if (p[1] == '&') {
			t = token__LOGAND;
			p++;
		} else if (p[1] == '=') {
			t = token__ASAND;
			p++;
		} else {
			t = token__AMPER;
		}
		p++;
		break;
	case '{':
		t = token__LBRACE;
		p++;
		break;
	case '}':
		t = token__RBRACE;
		p++;
		break;
	case '[':
		t = token__LBRACK;
		p++;
		break;
	case ']':
		t = token__RBRACK;
		p++;
		break;
	case '(':
		t = token__LPAREN;
		p++;
		break;
	case ')':
		t = token__RPAREN;
		p++;
		break;
	case '#':
		t = token__HASHTAG;
		p++;
		if (*p == '#') {
			t = token__DOUBLEHASH;
			p++;
		}
		break;
	case ':':
		t = token__COLON;
		p++;
		break;
	case ';':
		t = token__SEMI;
		p++;
		break;
	case ',':
		t = token__COMMA;
		p++;
		break;
	case '~':
		t = token__TILDE;
		p++;
		break;
	case '=':
		if (p[1] == '=') {
			t = token__EQUAL;
			p++;
		} else {
			t = token__ASSIGN;
		}
		p++;
		break;
	case '+':
		if (p[1] == '=') {
			t = token__ASPLUS;
			p++;
		} else if (p[1] == '+') {
			t = token__INCR;
			p++;
		} else {
			t = token__PLUS;
		}
		p++;
		break;
	case '-':
		if (p[1] == '=') {
			t = token__ASMINUS;
			p++;
		} else if (p[1] == '>') {
			t = token__ARROW;
			p++;
		} else if (p[1] == '-') {
			t = token__DECR;
			p++;
		} else {
			t = token__MINUS;
		}
		p++;
		break;
	case '>':
		if (p[1] == '=') {
			t = token__GTEQ;
			p++;
		} else if (p[1] == '>') {
			if (p[2] == '=') {
				t = token__ASRSHIFT;
				p++;
			} else {
				t = token__RSHIFT;
			}
			p++;
		} else {
			t = token__GREATER;
		}
		p++;
		break;
	case '<':
		if (p[1] == '=') {
			t = token__LTEQ;
			p++;
		} else if (p[1] == '<') {
			if (p[2] == '=') {
				t = token__ASLSHIFT;
				p++;
			} else {
				t = token__LSHIFT;
			}
			p++;
		} else {
			t = token__LESS;
		}
		p++;
		break;
	
	default:
		return -1;
	}
	self->current->next = token__new(t, b);
	self->current = self->current->next;
	self->ptr = p;
	return 0;
}

int lexer__tokenize(struct lexer *self, struct buf* buf, int offset, char *file)
{
	self->offset = offset;
	self->buf = buf;
	self->ptr = buf->buf;
	self->file = file;
	self->line = 0;

	lexer__trigraphs(self);
	while (!lexer__next(self)) {
	}
	if (self->ptr[0] != '\0') {
		return -1;
	}
	return 0;
}

int lexer__get_line_pos(struct lexer *self, struct token *tk)
{
	char *p;
	char *b;
	int l = 1;
	b = self->buf->buf;
	p = tk->value;	
	while (b < p) {
		if (*b == '\n') {
			l++;
		}
		b++;
	}
	return l;
}

char *lexer__get_value(struct lexer *self, struct token *tk)
{
	char *p;
	int l;

	if (!tk) {
		return "PANIC!";
	}
	buf__clear(self->tmp);

	switch(tk->type) {
	case token__END_OF_FILE: p = "EOF"; break;
	case token__END_OF_RULE: p = "EOR"; break;
	case token__MUL: p = "*"; break;
	case token__QMARK: p = "?"; break;
	case token__PIPE: p = "|"; break;
	case token__LPAREN: p = "("; break;
	case token__RPAREN: p = ")"; break;
	case token__COLON: p = ":"; break;
	case token__SEMI: p = ";"; break;
	case token__EQUAL: p = "="; break;
	default:
		p = tk->value;	
	}
	l = 0;
	while (p[l]) {
		if (p[l] == '\\' && p[l+1] != '\0') {
			l++;
		} else if (p[0] == '"') {
		       	if (l > 0 && p[l] == '"') {
				l++;
				break;
			}
		} else if (p[0] == '\'') {
		       	if (l > 0 && p[l] == '\'') {
				l++;
				break;
			}
		} else if ((p[l] >= '0' && p[l] <= '9') ||
			  (p[l] >= 'a' && p[l] <= 'z') ||
			  (p[l] >= 'A' && p[l] <= 'Z'))
		{
			/* dummy */
		} else {
			break;
		}	
		l++;
	}
	buf__append_txt(self->tmp, p, l);
	return buf__getstr(self->tmp);
}


