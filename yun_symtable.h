/**
 #file: yun_symtable.h
 #brief: symbol table
 #date: Oct 5, 2011
 #author: pozen
 */

#ifndef __YUN_SYMTABLE_H_
#define __YUN_SYMTABLE_H_

#include <stddef.h>

/* symbol type */
typedef enum SymType { FUNC, INT, UINT, SHORT, USHORT, CHAR, UCHAR, LONG,\
	ULONG, LONGLONG, ULONGLONG, FLOAT, DOUBLE, STRING, CONST }SymType;

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
		char *str_val;
		void *addr_val;
		struct
		{
			struct SymValue *addr;
			size_t size;
		};
	};
}SymValue;

typedef struct Symbol
{
	char *name;
	struct SymValue value;
	SymType type;
	int index;
	struct Symbol *next;
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
