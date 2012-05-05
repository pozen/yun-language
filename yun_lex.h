/**
 * @file: yun_lex.h
 * @brief: lexical analysis
 * @date: Oct 2, 2011
 * @author: pozen
 */

#ifndef __YUN_LEX_H_
#define __YUN_LEX_H_

#include <stddef.h>
#include <limits.h>

#define MAX_VALUE_LEN 2048

/* token types table */
enum RESERVED
{
	TK_ERROR = 256, TK_AND, TK_BREAK, TK_CONTINUE, TK_DO,\
	TK_ELIF, TK_ELSE, TK_FALSE, TK_FOR, TK_FUNCTION,\
	TK_IF, TK_IN, TK_NIL, TK_NOT, TK_OR, TK_OD,\
	TK_RETURN, TK_TRUE, TK_WHILE, Tk_CONST,\
	/*TK_INT, TK_LONG, TK_LONGLONG, TK_UINT, TK_ULONG, TK_ULONGLONG, TK_SHORT, TK_USHORT,\
	TK_FLOAT, TK_DOUBLE,*/TK_NUMBER, TK_CHAR, TK_STRING, \
	TK_EQ, TK_GE, TK_LE, TK_NE, TK_ID, TK_EOF, TK_CONST_VALUE
};

typedef double yl_number;
typedef char   yl_char;
#define reserved_value_type_test( tk ) ( tk >= TK_NUMBER && tk <= TK_STRING )

typedef void(*lex_error)( size_t lineno, const char* msg, ... );
typedef void(*lex_warning)( size_t lineno, const char* msg, ... );

typedef struct Token
{
	int type;
	int sub_type;
	char* value;
}Token;

typedef struct LexToken
{
	size_t line_num;
	/*size_t current;*/
	struct Token token;
	struct LexToken* next;
	struct LexToken* pre;
	/*char* source;
	//lex_error error;
	//lex_warning warning;*/
}LexToken;

/* ORDER RESERVED */
//const Token key_tokens[21];

/* get token list */
LexToken* lex_get_token( const char* source );

/* destroy token list */
void lex_destroy_token( LexToken* tk );

yl_number str2number( const char* str );


#endif /* __YUN_LEX_H_ */
