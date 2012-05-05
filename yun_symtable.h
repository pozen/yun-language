/**
 * pozen@yl:~>file 'yun_symtable.h'
 * pozen@yl:~>brief 'symbol table'
 * pozen@yl:~>date 'Oct 5, 2011'
 * pozen@yl:~>author 'pozen'
 */

#ifndef __YUN_SYMTABLE_H_
#define __YUN_SYMTABLE_H_

//#include <stddef.h>
#include "yun_lex.h"

/* symbol type */
//typedef enum SymType { FUNC = 0, UINT, USHORT, UCHAR,\
	ULONG, LONGLONG, ULONGLONG, CHAR, INT, SHORT, LONG, FLOAT, DOUBLE, STRING, CONST }SymType;
typedef enum SymType { FUNC = 0, NUMBER, CHAR, STRING, CONST }SymType;

/* symbol value */
typedef struct SymValue
{
	union
	{
		int int_val;
		//uint uint_val;
		short short_val;
		//ushort ushort_val;
		char char_val;
		//uchar uchar_val;
		long long_val;
		//ulong ulong_val;
		long long ll_val;
		//unsigned long long  ull_val;
		float float_val;
		double double_val;
		yl_number _number;
		char *str_val;
		struct /*for array*/
		{
			void *addr;
			short unit_size;
			short len;
		};
	};
}SymValue;

typedef struct Symbol
{
	char *name; /*0 == const type*/
	struct SymValue value;
	SymType type;
	unsigned int index;
	int offset;
	struct Symbol *addr;
	struct Symbol *next;
	void *func_info;
}Symbol;

#define SYMBOL_TABLE_LEN 257 /* hash */

typedef struct SymbolTable
{
	size_t len;
	Symbol *table[SYMBOL_TABLE_LEN];
}SymbolTable;

Symbol *sym_create_node( char *name, SymValue value, size_t type );
void sym_destroy_node( Symbol *node );




#endif /* YUN_SYMTABLE_H_ */
