/**
 * pozen@yl:~>file 'yun_parser.h'
 * pozen@yl:~>brief 'syntax analysis'
 * pozen@yl:~>date 'Oct 3, 2011'
 * pozen@yl:~>author 'pozen'
 */

#ifndef __YUN_PARSER_H_
#define __YUN_PARSER_H_

#include "yun_lex.h"
#include "yun_symtable.h"

/* syntax node type : statement & expression & domain */
typedef enum { STMT, EXP, DOMAIN } ;

/* stmt type */
typedef enum {
	VAR_DEF, CONTINUE, BREAK, RETURN, ASSIGN
}StmtType;

/* exp type */
typedef enum {
	ID, CONST_VAL, OP, FUNC_CALL
}ExpType;

/* domain type */
typedef enum {
	GLOBAL, FUNC_DEF, FOR, WHILE, IF, ELIF, ELSE, MAIN
}DomainType;

typedef struct priority{
	int type;
	int prior;
}priority;


const priority prior_table[];

/* node of syn tree */
typedef struct SynNode{
	int lineno;
	int type;
	union{
		StmtType stmt;
		ExpType exp;
	}subType;
	int op;
	int rnum;
	int offset;
	Symbol *rsymbol;
    Symbol node_sym;
    Symbol *sym_table; /* for domain node */
    Symbol *sym_table_const;
	struct SynNode* parent; /* ONLY for searching domain */
	struct SynNode* lchild;
	struct SynNode* rchild;
	struct SynNode* sibling;
}SynNode;

typedef struct SynState
{
	LexToken* pos;
	//error
}SynState;

/* parse a token list */
SynNode *syn_parse( SynState *ss );

int operator_check( int type );

/* destroy a syntax tree */
void syn_destroy( SynNode* tree );

#endif /* __YUN_PARSER_H_ */
