
#ifndef TOKEN_H_
#define TOKEN_H_

#define token__CLOSED 0x01

#define token__NONE 0x00010000
#define token__LEFT 0x00020000
#define token__RIGHT 0x00030000
#define token__GROUP 0x00080000
#define token__AMASK 0x000F0000
#define token__PMASK 0x7FF00000
#define token__P(a) ((a << 20) & token__PMASK)
enum
{
	token__ROOT = 0,
	token__OP1 = 1,
	token__OP2 = 2,
	token__TERM = 3,
	token__LPAREN = 0x400 + token__P(17) + token__LEFT + token__GROUP,
	token__RPAREN = 0x400 + token__P(17) + token__GROUP,
	token__LBRACE = 0x401 + token__P(0x7FF) + token__LEFT + token__GROUP,
	token__RBRACE = 0x401 + token__P(0x7FF) + token__GROUP,
	token__LCAST = 0x402 + token__P(16) + token__RIGHT + token__GROUP,
	token__RCAST = 0x402 + token__P(16) + token__GROUP,
	token__LBRACK = 0x403 + token__P(17) + token__LEFT + token__GROUP,
	token__RBRACK = 0x403 + token__P(17) + token__GROUP,

	token__NOTEQ = 0x600 + token__P(10) + token__LEFT,
	token__XMARK = 0x601 + token__P(10) + token__RIGHT,
	token__ASMOD = 0x602 + token__P(3) + token__RIGHT,
	token__MOD = 0x603 + token__P(14) + token__LEFT,
	token__LOGAND = 0x604 + token__P(6) + token__LEFT,
	token__ASAND = 0x605 + token__P(3) + token__RIGHT,
	token__AMPER = 0x606 + token__P(16) + token__RIGHT,
	token__BITWISEAND = 0x607 + token__P(9) + token__LEFT,
	token__ASMUL = 0x608 + token__P(3) + token__RIGHT,
	token__MUL = 0x609 + token__P(14) + token__LEFT,
	token__STAR = 0x60A + token__P(16) + token__RIGHT,
	token__INCR = 0x60B + token__P(16) + token__RIGHT,
	token__POSTINCR = 0x60C + token__P(17) + token__LEFT,
	token__PLUS = 0x60D + token__P(16) + token__RIGHT,
	token__ADD = 0x60E + token__P(13) + token__LEFT,
	token__COMMA = 0x60F + token__P(1) + token__LEFT,
	token__DECR = 0x610 + token__P(16) + token__RIGHT,
	token__POSTDECR = 0x611 + token__P(17) + token__LEFT,
	token__ASMINUS = 0x612 + token__P(3) + token__RIGHT,
	token__ARROW = 0x613 + token__P(17) + token__LEFT,
	token__MINUS = 0x614 + token__P(16) + token__RIGHT,
	token__SUB = 0x615 + token__P(13) + token__LEFT,
	token__ASDIV = 0x616 + token__P(3) + token__RIGHT,
	token__DIV = 0x617 + token__P(17) + token__LEFT,
	token__COLON = 0x618 + token__P(4) + token__RIGHT,
	token__QMARK = 0x619 + token__P(4) + token__RIGHT,
	token__SEMI = 0x61A +  token__NONE,
	token__ASLSHIFT = 0x61B + token__P(4) + token__RIGHT,
	token__LSHIFT = 0x61C + token__P(12) + token__LEFT,
	token__LTEQ = 0x61D + token__P(11) + token__LEFT,
	token__LESS = 0x61E + token__P(11) + token__LEFT,
	token__EQUAL = 0x61F + token__P(10) + token__LEFT,
	token__ASSIGN = 0x620 + token__P(3) + token__RIGHT,
	token__ASRSHIFT = 0x621 + token__P(4) + token__RIGHT,
	token__RSHIFT = 0x622 + token__P(12) + token__LEFT,
	token__GTEQ = 0x623 + token__P(11) + token__LEFT,
	token__GREATER = 0x624 + token__P(11) + token__LEFT,
	token__ASXOR = 0x625 + token__P(3) + token__RIGHT,
	token__CARET = 0x626 + token__P(8) + token__LEFT,
	token__LOGOR = 0x627 + token__P(5) + token__LEFT,
	token__ASOR = 0x628 + token__P(3) + token__RIGHT,
	token__PIPE = 0x629 + token__P(7) + token__LEFT,
	token__TILDE = 0x62A + token__P(16) + token__RIGHT,
	token__ELLIPSIS = 0x62B +  token__NONE,
	token__DOT = 0x62C + token__P(17) + token__LEFT,
	token__SIZEOF = 0x62D + token__P(10) + token__NONE,
	token__HASHTAG = 0x62E,
	token__DOUBLEHASH = 0x62F,
	token__ASPLUS = 0x630 + token__P(3) + token__RIGHT,

	token__IDENTIFIER = 0x700,
	token__AUTO = 0x701,
	token__BREAK = 0x702,
	token__CASE = 0x703,
	token__CHAR = 0x704,
	token__CONST = 0x705,
	token__CONTINUE = 0x706,
	token__DEFAULT = 0x707,
	token__DO = 0x708,
	token__DOUBLE = 0x709,
	token__DEFINED = 0x70A,
	token__ELSE = 0x70B,
	token__ELIF = 0x70C,
	token__ENUM = 0x70D,
	token__EXTERN = 0x70E,
	token__ERROR = 0x70F,
	token__FLOAT = 0x710,
	token__FOR = 0x711,
	token__GOTO = 0x712,
	token__INT = 0x713,
	token__IF = 0x714,
	token__IFDEF = 0x715,
	token__IFNDEF = 0x716,
	token__LONG = 0x717,
	token__LINE = 0x718,
	token__PRAGMA = 0x719,
	token__REGISTER = 0x71A,
	token__RETURN = 0x71B,
	token__SHORT = 0x71C,
	token__SIGNED = 0x71D,
	token__STATIC = 0x71E,
	token__STRUCT = 0x71F,
	token__SWITCH = 0x720,
	token__TYPEDEF = 0x721,
	token__UNION = 0x722,
	token__UNDEF = 0x723,
	token__UNSIGNED = 0x724,
	token__VOID = 0x725,
	token__VOLATILE = 0x726,
	token__WHILE = 0x727,
	token__DEFINE = 0x728,
	token__INCLUDE = 0x729,
	token__END_OF_RULE = 0x800,
	token__PRODUCTION = 0x900,
	token__CONSTANT = 0xA00,
	token__END_OF_FILE = 0xB00,
	token__GLUE = 0x0000FFFE + token__P(2) + token__LEFT,
	token__PLACEHOLDER = 0x0000FFFF + token__P(1)
};

struct token
{
	struct token *next;
	int type;
	int offset;
	char *value;
};

struct token *token__new(int type, char *value);
int token__dispose(struct token *token);

#endif /* TOKEN_H_ */
