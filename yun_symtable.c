/**
 * @file: yun_symtable.c
 * @brief: symbol table
 * @date: Oct 5, 2011
 * @author: pozen
 */

#include <malloc.h>
#include <memory.h>
#include <string.h>
#include "yun_symtable.h"

Symbol *sym_create_node( char *name, SymValue value, size_t type )
{
	Symbol *node = (Symbol*)malloc( sizeof( Symbol ) );
	node->name = (char*)malloc( strlen( name ) + 1 );
	strcpy( node->name, name );
	node->value = value;
	node->type = type;
	node->next = 0;
	return node;
}

void sym_destroy_node( Symbol *node ){
	Symbol *tmp;
	while( node ){
		tmp = node->next;
		free( node );
		node = tmp;
	}
}

