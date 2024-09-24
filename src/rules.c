#include "parser.h"
#include "token.h"
#include "ast.h"

/*
translation_unit:	external_declaration*
*/
static int external_declaration(struct parser *self, int mode);

int translation_unit_1(struct parser *self, int mode)
{
	if (self->status || parser__is_loop(self)) {
		/* failure, this is the end of the main loop */
		self->status = 0;
		return 0;
	}
	printf("%s\n", self->tk->value);
	/* push again ourself, so we create a loop until it fails */
	parser__push(self, translation_unit_1, 0);
	parser__push(self, external_declaration, 1);
	return 0;
}

int translation_unit(struct parser *self, int mode)
{
	if (self->tk->type != token__ROOT) {
		parser__error(self, "Bad input", 0);
		return -1;
	}
	parser__eat(self);
	parser__push(self, translation_unit_1, 0);
	parser__push(self, external_declaration, 1);
	return 0;
}

/*
external_declaration:	function_definition | declaration
*/
static int function_definition(struct parser *self, int mode);
static int declaration(struct parser *self, int mode);

static int external_declaration_1(struct parser *self, int mode)
{
	if (self->status > 0) {
		self->status = 0;
		parser__push(self, declaration, mode);
	}
	return 0;
}

static int external_declaration(struct parser *self, int mode)
{
	parser__push(self, external_declaration_1, mode);
	parser__push(self, function_definition, 1); /* mode 1: probe before execute */
	return 0;
}

/*
function_definition:	declaration_specifiers? declarator 
			declaration_list? compound_statement
*/
static int declaration_specifiers(struct parser *self, int mode);
static int declarator(struct parser *self, int mode);
static int declaration_list(struct parser *self, int mode);
static int compound_statement(struct parser *self, int mode);

static int function_definition(struct parser *self, int mode)
{
	int r = 0;
	if (mode == 1) {

	}
	return r;
}

/*
declaration:		declaration_specifiers init_declaration_list? ";"
*/
static int init_declaration_list(struct parser *self, int mode);
static int declaration_1(struct parser *self, int mode)
{
	if (self->tk->type != token__SEMI) {
		parser__error(self, "missing ';'", mode);
	}
	parser__eat(self);
	return 0;
}

static int declaration(struct parser *self, int mode)
{
	parser__push(self, declaration_1, mode);
	parser__push(self, init_declaration_list, 1);
	parser__push(self, declaration_specifiers, mode);
	return 0;
}

/*
declaration_list:	declaration declaration*
*/
/*
declaration_specifiers:	(storage_class_specifier | type_specifier | 
			type_qualifier)  (storage_class_specifier | 
			type_specifier | type_qualifier)*
*/
static int declaration_specifiers(struct parser *self, int mode)
{
}

/*
storage_class_specifier:	"auto" | "register" | "static" | "extern" |
				"typedef"
*/
/*
type_specifier:		"void" | "char" | "short" | "int" | "long" | "float" |
			"double" | "signed" | "unsigned" |
			struct_or_union_specifier | enum_specifier  |
			typedef_name
*/
/*
type_qualifier:		"const" | "volatile"
*/
/*
struct_or_union_specifier:	struct_or_union 
				(identifier? "{" struct_declaration_list "}") |
				identifier
*/
/*
struct_or_union:	"struct" | "union"
*/
/*
struct_declaration_list:	struct_declaration struct_declaration*
*/
/*
init_declaration_list:	init_declarator ( "," init_declarator)*
*/
static int init_declaration_list(struct parser *self, int mode)
{
}

/*
init_declarator:	declarator ("=" initializer)?
*/
/*
struct_declaration:	specifier_qualifier_list struct_declarator_list ";"
*/
/*
specifier_qualifier_list:	(type_specifier | type_qualifier)
				(type_specifier | type_qualifier)*
*/
/*
struct_declarator_list:		struct_declarator ( "," struct_declarator)*
*/
/*
struct_declarator:	declarator | 
			(declarator? ":" constant_expression)
*/
/*
enum_specifier:		"enum" identifier | 
			(identifier? "{" enumerator_list "}")
*/
/*
enumerator_list:	enumerator ( "," enumerator)*
*/
/*
enumerator:		identifier ("=" constant_expression)?
*/
/*
declarator:		pointer? direct_declarator
*/
/*
direct_declarator:	direct_declarator1 | direct_declarator2
*/
/*
direct_declarator2:	(direct_declarator "[" constant_expression? "]") |
			(direct_declarator "(" parameter_type_list  ")") |
			(direct_declarator "(" identifier_list?  ")") 
*/
/*
direct_declarator1:	identifier | ("(" declarator ")")
*/
/*
pointer:		"*" type_qualifier_list? ("*" type_qualifier_list?)*
*/
/*
type_qualifier_list:	type_qualifier type_qualifier*
*/
/*
parameter_type_list:	parameter_list ( "," "...")?
*/
/*
parameter_list:		parameter_declaration ( "," parameter_declaration)*
*/
/*
parameter_declaration:	declaration_specifiers 
			declarator | abstract_declarator?
*/
/*
identifier_list:	identifier ( "," identifier)*
*/
/*
initializer:		assignment_expression |
			("{" initializer_list  ","? "}")
*/
/*
initializer_list:	initializer ( "," initializer)*
*/
/*
type_name:		specifier_qualifier_list abstract_declarator?
*/
/*
abstract_declarator:	pointer | (pointer? direct_abstract_declarator)
*/
/*
direct_abstract_declarator:	("(" abstract_declarator ")") |
				(("(" abstract_declarator ")")? 
				  (("[" constant_expression? "]") |
				   ("(" parameter_type_list? ")")))*
*/
/*
typedef_name:		identifier
*/
/*
statement:		labeled_statement |
			expression_statement |		
			compound_statement |		
			selection_statement |		
			iteration_statement |		
			jump_statement
*/
/*
labeled_statement:	(identifier ":" statement) |
			("case" constant_expression ":" statement) |
			("default" ":" statement)
*/
/*
expression_statement:	expression? ";"
*/
/*
compound_statement:	"{" declaration_list? statement_list? "}"
*/
/*
statement_list:		statement statement*
*/
/*
selection_statement:	("if" "(" expression ")" statement 
			       ("else" statement)?) |
			("switch" "(" expression ")" statement)
*/
/*
iteration_statement:	("while" "(" expression ")" statement) |
			("do" statement "while" "(" expression ")" ";") |
			("for" "(" expression? ";" expression? ";" expression?
			  ")" statement)
*/
/*
jump_statement:		("goto" identifier ";") |
			("continue" ";") |
			("break" ";") |
			("return" expression ";")
*/
/*
expression:		assignment_expression ( "," assignment_expression)* 
*/
/*
assignment_expression:	conditional_expression |
			((unary_expression assignment_operator)* 
			  conditional_expression)
*/
/*
assignment_operator:	"=" | "*=" | "/=" | "%=" | "+=" | "-=" | "<<=" |
			">>=" | "&=" | "^=" | "|="
*/
/*
conditional_expression:	logical_or_expression 
			("?" expression ":" ("?" expression ":")* 
			  logical_or_expression)? 
*/
/*
constant_expression:	conditional_expression
*/
/*
logical_or_expression:	logical_and_expression ("||" logical_and_expression)*
*/
/*
logical_and_expression:	inclusive_or_expression | 
			("&&" inclusive_or_expression)* 
*/
/*
inclusive_or_expression:	exclusive_or_expression 
				("|" exclusive_or_expression)*
*/
/*
exclusive_or_expression:	and_expression ("^" and_expression)*
*/
/*
and_expression:		equality_expression ("&" equality_expression)*
*/
/*
equality_expression:	relational_expression 
			("==" | "!=" relational_expression)*
*/
/*
relational_expression:	shift_expression 
			("<" | ">" | "<=" | ">=" shift_expression)*
*/
/*
shift_expression:	additive_expression ("<<" | ">>" additive_expression)*
*/
/*
additive_expression:	multiplicative_expression 
			("+" | "-" multiplicative_expression)*
*/
/*
multiplicative_expression:	cast_expression
				("*" | "/" | "%" cast_expression)
*/
/*
cast_expression:	("(" type_name ")")* unary_expression
*/
/*
unary_expression:	unary_expression1 | unary_expression2
*/
/*
unary_expression1:	postfix_expression |
			(unary_operator cast_expression) |
			("sizeof" "(" type_name ")")
*/
/*
unary_expression2:	"++" | "--" | "sizeof" unary_expression 
*/
/*
unary_operator:		"&" | "*" | "+" | "-" | "~" | "!"
*/
/*
postfix_expression:	primary_expression |
			(postfix_expression
			 ("[" expression "]") |
			 ("(" argument_expression_list? ")") |
			 ("." identifier) |
			 ("->" identifier) |
			 "++" | "--")
*/
/*
primary_expression:	identifier | constant | string_literal | 
			("(" expression ")")
*/
/*
argument_expression_list:	assignment_expression 
				( "," assignment_expression)*
*/
/*
constant:		integer_constant | character_constant |
			floating_constant | enumeration_constant
*/
/*
//integer_constant:
//character_constant:
//floating_constant: 
//enumeration_constant: 
//string_literal:
//identifier:
*/

