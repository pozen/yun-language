/**
 * pozen@yl:~>file 'yun_lex.c'
 * pozen@yl:~>brief 'lex analysis'
 * pozen@yl:~>date 'Oct 2, 2011'
 * pozen@yl:~>author 'pozen'
 */

#include "yun_lex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

/* reserved words */
const Token key_tokens[] =
{
		{ TK_AND, 0, "and" },
		{ TK_BREAK, 0, "break" },
		{ TK_CONTINUE, 0, "continue" },
		{ TK_DO, 0, "do" },
		{ TK_ELIF, 0, "elif" },
		{ TK_ELSE, 0, "else" },
		{ TK_FALSE, 0, "false" },
		{ TK_FOR, 0, "for" },
		{ TK_FUNCTION, 0, "function" },
		{ TK_IF, 0, "if" },
		{ TK_IN, 0, "in" },
		{ TK_NIL, 0, "nil" },
		{ TK_NOT, 0, "not" },
		{ TK_OD, 0, "od" },
		{ TK_OR, 0, "or" },
		{ TK_RETURN, 0, "return" },
		{ TK_TRUE, 0, "true" },
		{ TK_WHILE, 0, "while" },
		{ TK_INT, 0, "int" },
		{ TK_LONG, 0, "long" },
		{ TK_LONGLONG, 0, "llong" },
		{ TK_UINT, 0, "uint" },
		{ TK_ULONG, 0, "ulong" },
		{ TK_ULONGLONG, 0, "ullong" },
		{ TK_SHORT, 0, "short" },
		{ TK_USHORT, 0, "ushort" },
		{ TK_FLOAT, 0, "float" },
		{ TK_DOUBLE, 0, "double" },
		{ Tk_CONST, 0, "const" },
		{ 0, 0, "NULL" }
};

#define LEX_TOKEN_SIZE sizeof( LexToken )

static LexToken* lex_add_token( LexToken* cur, int lineno, int token_type, int token_sub_type, const char* value )
{
	cur->token.type = token_type;
	cur->token.sub_type = token_sub_type;
	if( 0 != value )
	{
		cur->token.value = (char*)malloc( strlen( value ) + 1 );
		strcpy( cur->token.value, value );
	}
	else cur->token.value = 0;
	cur->line_num = lineno;
	LexToken* newlt = (LexToken*)malloc( LEX_TOKEN_SIZE );
	cur->next = newlt;
	newlt->pre = cur;
    return newlt;
}

static LexToken* lex_read_char( LexToken* cur, int lineno, const char* src, int* index )
{
	char c1 = src[*index];
	char c2 = src[*index+1];
	if( '\'' != c1 && '\'' == c2 )
	{
		char tmp[2];
		tmp[0] = c1, tmp[1] = 0;
		*index = *index + 1;
		return lex_add_token( cur, lineno, TK_CONST_VALUE, TK_CHAR, tmp );
	}
	char c3 = src[*index+2];
	if( '\\' == c1 && ( 'n' == c2 || 't' ) && '\'' == c3 )
	{
		char tmp[2];
		if( 'n' == c2 )tmp[0] = 10;
		else tmp[0] = 9;
		tmp[1] = 0;
		*index = *index + 2;
		return lex_add_token( cur, lineno, TK_CONST_VALUE, TK_CHAR, tmp );
	}
	else return lex_add_token( cur, lineno, TK_ERROR, 0, 0 );
}

static LexToken* lex_read_string( LexToken* cur, int lineno, const char* src, int* index )
{
	char tmp[MAX_VALUE_LEN + 1];
	size_t len = 0;
	while( '\n' != src[*index] && '"' != src[*index] && len < MAX_VALUE_LEN)
	{
		if( '\\' == src[*index] && ( 'n' == src[*index+1] || 't' == src[*index+1] ) )
		{
			if( 'n' == src[*index+1] ) tmp[len++] = 10;
			else tmp[len++] = 9;
			*index = *index + 2;
		}
		else tmp[len++] = src[*index], *index = *index + 1;
	}
	if( len >= MAX_VALUE_LEN ) return lex_add_token( cur, lineno, TK_ERROR, 0, 0 );
	else
	{
		tmp[len] = 0;
		return lex_add_token( cur, lineno, TK_CONST_VALUE, TK_STRING, tmp );
	}
}

static LexToken* lex_read_number( LexToken* cur, int lineno, const char* src, int* index )
{
	char tmp[MAX_VALUE_LEN + 1];
	int len = 0;
	int flag = 0;
	*index = *index - 1;
	while( src[*index] >= '0' && src[*index] <= '9' && len < MAX_VALUE_LEN )
	{
		tmp[len++] = src[*index], *index = *index + 1;
		if( 0 == flag && '.' == src[*index] )
			tmp[len++] = '.', *index = *index + 1, flag++;
	}
	tmp[len] = 0;
	return lex_add_token( cur, lineno, TK_CONST_VALUE, flag ? TK_FLOAT : TK_INT, tmp );
}

static LexToken* lex_read_id( LexToken* cur, int lineno, const char* src, int* index )
{
	char tmp[MAX_VALUE_LEN + 1];
	int len = 0;
	int i = 0;
	tmp[len++] = src[*index - 1];
	while( src[*index] >= '0' && src[*index] <= '9' || src[*index] >= 'A' && src[*index] <= 'z' || src[*index] == '_')
		tmp[len++] = src[*index], *index = *index + 1;
	tmp[len] = 0;
	for( ; key_tokens[i].type != 0 ; ++i )
		if( strcmp( key_tokens[i].value, tmp ) == 0 )
			return lex_add_token( cur, lineno, key_tokens[i].type, 0, tmp );
	return lex_add_token( cur, lineno, TK_ID, 0, tmp );
}

LexToken* lex_get_token( const char* source )
{
	int index = 0;
	int lineno = 1;
	char c = source[index++];
	if( EOF == c )
		return 0;
	LexToken* first = (LexToken*)malloc( LEX_TOKEN_SIZE );
	LexToken* cur = first;
	cur->pre = NULL;
	for( ; c != EOF && c; c = source[index++] )
	{
		switch( c )
		{
		case '\n':
		case '\r':
			lineno++;
			break;
		case ' ':
		case '\t':
			break;
		case '>':
			if( '=' == source[index] )
			{
				cur = lex_add_token( cur, lineno, TK_GE, 0, ">=" );
				index++;
			}
			else
				cur = lex_add_token( cur, lineno, '>', 0, ">" );
			break;
		case '<':
			if( '=' == source[index] )
			{
				cur = lex_add_token( cur, lineno, TK_LE, 0, "<=" );
				index++;
			}
			else
				cur = lex_add_token( cur, lineno, '<', 0, "<" );

			break;
		case '=':
			if( '=' == source[index] )
			{
				cur = lex_add_token( cur, lineno, TK_EQ, 0, "==" );
				index++;
			}
			else
				cur = lex_add_token( cur, lineno, '=', 0, "=" );
			break;
		case '!':
			if( '=' == source[index] )
				cur = lex_add_token( cur, lineno, TK_NE, 0, "!=" );
			else
				cur = lex_add_token( cur, lineno, '!', 0, "!" );
			index++;
			break;
#define SINGLE_TOKEN( ch, value ) case ch:\
		    cur = lex_add_token( cur, lineno, ch, 0, value );\
			break
		SINGLE_TOKEN( '|', "|" );
		SINGLE_TOKEN( '+', "+" );
		SINGLE_TOKEN( '-', "-" );
		SINGLE_TOKEN( '*', "*" );
		SINGLE_TOKEN( '/', "/" );
		SINGLE_TOKEN( ',', "," );
		SINGLE_TOKEN( ';', ";" );
		SINGLE_TOKEN( '(', "(" );
		SINGLE_TOKEN( ')', ")" );
		SINGLE_TOKEN( '{', "{" );
		SINGLE_TOKEN( '}', "}" );
		case '\'':
			cur = lex_read_char( cur, lineno, source, &index );
			index++;
			break;
		case '"':
			cur = lex_read_string( cur, lineno, source, &index );
			index++;
			break;
		default:
			if( c >= '0' && c <= '9' )
				cur = lex_read_number( cur, lineno, source, &index );
			else if( c >= 'A' && c <= 'z' || '_' == c )
				cur = lex_read_id( cur, lineno, source, &index );
			else cur = lex_add_token( cur,lineno, TK_ERROR, 0, 0 );
		}
	}
	cur->pre->next = 0;
	free(cur);
	return first;
}

void lex_destroy_token( LexToken* tk )
{
	LexToken* tmp;
	while( tk )
	{
		free( tk->token.value );
		tmp = tk->next;
		free( tk );
		tk = tmp;
	}
}

