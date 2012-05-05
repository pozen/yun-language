/**
 * pozen@yl:~>file 'yun_parser.c'
 * pozen@yl:~>brief 'syntax analysis'
 * pozen@yl:~>date 'Oct 15, 2011'
 * pozen@yl:~>author 'pozen'
 */

#include "yun_parser.h"
#include <stdio.h>
#include <memory.h>
#include <malloc.h>

#define SYN_NODE_SIZE sizeof ( SynNode )

const priority prior_table[] = {
		{ TK_OR, 20 },
		{ TK_AND, 21 },
		{ '|', 22 },
		{ '^', 23 },
		{ '&', 24 },
		{ TK_NE, 25},
		{ TK_EQ, 25 },
		{ TK_LE, 26 },
		{ TK_GE, 26 },
		{ '>', 26 },
		{ '<', 26 },
		{ '+', 27 },
		{ '-', 27 },
		{ '%', 28 },
		{ '*', 28 },
		{ '/', 28 },
		//{ '(', 29 },
		{ 0, 0 }
};

size_t  global_id_num;

static SynNode *syn_id_def_parse( SynState *ss, SynNode *parent, int step, int type, int const_flag );
static SynNode *syn_exp_parse( SynState *ss, SynNode *parent );
static SynNode *syn_domain_parse( SynState *ss, SynNode *parent );

static int syn_prior_cmp( int type1, int type2 )
{
	int v1 = 0, v2 = 0, i = 0;
	for( ; prior_table[i].type != 0; i++ )
	{
		if( prior_table[i].type == type1 )
			v1 = prior_table[i].prior;
		if( prior_table[i].type == type2 )
			v2 = prior_table[i].prior;
	}
	if( 0 == v1 || 0 == v2 ) return -2;
	return v1 > v2 ? 1 : v1 == v2 ? 0 : -1;
}

static SynNode *syn_adjust_prior( SynNode *node )
{
	SynNode *c = node->rchild, *p = node->parent, *tmp;
	if( !c )
		return node;
	if( syn_prior_cmp( node->subType.exp, c->subType.exp ) >= 0 )
	{
		tmp = c->lchild;
		c->lchild = node;
		node->rchild = tmp;
		node->parent = c;
		c->parent = p;
		//if( 0 != p ) p->rchild = c;
		//if( c->lchild ){
		//	if( c->lchild->rchild ) c->lchild = syn_adjust_prior( c->lchild );
		//}
		return c;
	}
	return node;
}

static void syn_token_next( SynState *ss ) { ss->pos = ss->pos->next; }
static int syn_token_type( SynState *ss ) { return ss->pos->token.type; }
static int syn_token_subtype( SynState *ss ) { return ss->pos->token.sub_type; }
static int syn_token_lineno( SynState *ss ) { return ss->pos->line_num; }
static char* syn_token_value( SynState *ss ) { return ss->pos->token.value; }
static void syn_insert_rchild ( SynNode *node, SynNode *child ) { node->rchild = child; if( child ) child->parent = node; }
static void syn_insert_lchild ( SynNode *node, SynNode *child ) { node->lchild = child; if( child ) child->parent = node; }

static SynNode *syn_create_node( int type, int subType, SynState *ss )
{
	SynNode *node = (SynNode*)malloc( sizeof ( SynNode ) );
	node->type = type;
	node->subType.stmt = subType;
	node->sibling = 0;
	node->lineno = syn_token_lineno( ss );
	node->parent = 0;
	node->lchild = 0;
	node->rchild = 0;
	node->sym_table = 0;
	node->node_sym.value.str_val = syn_token_value( ss );
	return node;
}

int my_strlen( char *str )
{
	int len = 0;
	if( !str ) return 0;
	while( str[len] != '\0' )
	{
		len++;
	}
	return len;
}

int my_strcmp( char *src, char *dst )
{
	int sl = my_strlen(src);
	int dl = my_strlen(dst);
	if(sl != dl)
		return -1;
	int i = 0;
	for( i = 0; i < sl; i++ )
		if( src[i] > dst[i] )
			return 1;
		else if(src[i] < dst[i])
			return -1;
	return 0;
}

static Symbol* syn_sym_qurry( SynNode *pos, char *name )
{
    while( pos )
    {
    	if( pos->type == DOMAIN )
    	{
    		Symbol *tmp = pos->sym_table;
    		while( tmp )
    		{
    	    	if( my_strcmp( tmp->name, name ) == 0 )
    	    		return tmp;
    	    	tmp  = tmp->next;
    		}
    	}
    	pos = pos->parent;
    }
    return 0;
}

static Symbol* syn_update_sym_table( SynNode *pos, char *name, int type, char *value )
{
	if( !pos )
		return -1;
	while( pos->type != DOMAIN && pos->parent )
		pos = pos->parent;
	if( pos->type != DOMAIN ) return -1;
    Symbol *sb = (Symbol*)malloc( sizeof( Symbol ) );
    Symbol *tmp = pos->sym_table;
    sb->name = name;
    sb->type = type;
    sb->value.str_val = value;
    if( 0 == name )//const
    {
    	if( type == TK_CHAR )
    		sb->value.char_val = value[0];
    	else if( type == TK_NUMBER )
    		sb->value._number = str2number( value );
    }
    sb->next = 0;
    sb->func_info = (void*)pos;
    sb->index = -1;
    if( 0 == pos->sym_table )
    {
    	sb->index = global_id_num++;
    	pos->sym_table = sb;
    	return sb;
    }
    int flag = 0;
    while( tmp )
    {
    	if( name && my_strcmp( tmp->name, name ) == 0 )
    		flag = 1;//error ID is defined multiple times in the same scope
    	if( 0 == tmp->next )
    		break;
    	tmp  = tmp->next;
    }
    if( !flag )
    {
    	tmp->next = sb;
    	sb->index = global_id_num++;//tmp->index + 1;
    }
   // return sb->index;
    return sb;
}

static SynNode *syn_id_def_parse( SynState *ss, SynNode *parent, int step, int type, int func_flag ) /* func_flag is for func_def parsing */
{
	SynNode *node = 0, *node2 = 0;
	if( syn_token_type( ss ) == TK_ID )
	{
		node = syn_create_node( STMT, VAR_DEF, ss ); node->parent = parent;
		node->rsymbol = syn_update_sym_table( node, syn_token_value( ss ), syn_token_type( ss ), 0 );
		syn_token_next( ss );
		if( syn_token_type( ss ) == '=' )
		{
			node2 = syn_create_node( STMT, ASSIGN, ss );
			node2->lchild = node;
			syn_token_next( ss );
			node2->rchild = syn_exp_parse( ss, parent ); node2->parent = parent;
			if( ';' == syn_token_type( ss ) )
			{
				syn_token_next( ss );
				return node2;
			}
			if( ',' == syn_token_type( ss ) )
			{
				syn_token_next( ss );
				node2->sibling = syn_id_def_parse( ss, parent, step + 1, type, func_flag );
				return node2;
			}
			else
				return node;//error
		}
		//else if( const_flag && (*ss)->token.type == Tk_CONST )
		//{
		//	;//error
		//}
		else if( syn_token_type( ss ) == ';' )
		{
			syn_token_next( ss );
			return node;
		}
		else if( syn_token_type( ss ) == ',' )
		{
			syn_token_next( ss );
			node->sibling = syn_id_def_parse( ss, parent, step + 1 , type, func_flag );//exp
			return node;
		}
		else
			;//error
	}
	else if( syn_token_type( ss ) == Tk_CONST )
	{
		syn_token_next( ss );
		if( 0 != step && !func_flag )
			return node;//error
		else
		{
			if( reserved_value_type_test( syn_token_type( ss ) ) )
			{
				syn_token_next( ss );
				node = syn_id_def_parse( ss, parent, step + 1 , ss->pos->pre->token.type, func_flag ); node->parent = parent;
				return node;
			}
			else if( TK_ID == syn_token_type( ss ) )
			{
				node = syn_id_def_parse( ss, parent, step + 1 , syn_token_type( ss ), func_flag ); node->parent = parent;
				return node;
			}
			else
				return node;//error
		}
	}
	else if( reserved_value_type_test( syn_token_type( ss ) ) )
	{
		syn_token_next( ss );
		if( 0 != step && !func_flag )
			return node;//error
		node = syn_id_def_parse( ss, parent, step + 1 , syn_token_type( ss ), func_flag ); node->parent = parent;
		return node;
	}
	else
		return node;//error
    return node;
}

static SynNode *syn_func_def_parse( SynState *ss, SynNode *parent )
{
	SynNode *node = 0;
	syn_token_next( ss );
	if( syn_token_type( ss ) != TK_ID )
		return node;
	node = syn_create_node( DOMAIN, FUNC_DEF, ss );
	node->rsymbol = syn_update_sym_table( node, syn_token_value( ss ), FUNC, 0 );
	syn_token_next( ss );
	if( syn_token_type( ss ) != '(' )
		;//error
	syn_token_next( ss );
	node->lchild = syn_id_def_parse( ss, node, 0, 0, 1 );
	if( syn_token_type( ss ) != ')' )
		;//error
	else
	{
		syn_token_next( ss );
		if( syn_token_type( ss ) != TK_DO )
			;//error
		syn_token_next( ss );
		node->rchild = syn_domain_parse( ss, node );//stmt parse
		if( syn_token_type( ss ) != TK_OD )
			;//error
		syn_token_next( ss );
	}
	return node;
}

static SynNode *syn_if_parse( SynState *ss, SynNode *parent, int type )
{
	SynNode *node, *node1, *node2;
	node = syn_create_node( DOMAIN, type, ss );
	node->parent = parent;
	syn_token_next( ss );
	if( type != TK_ELSE )
		syn_insert_lchild( node, syn_exp_parse( ss, node ) );
	if( syn_token_type( ss ) != TK_DO )
		;//error
	else
	{
		syn_token_next( ss );
		syn_insert_rchild( node, syn_domain_parse( ss, node ) );
		if( syn_token_type( ss ) != TK_OD )
			;//error
		syn_token_next( ss );
	}
	return node;
}

static SynNode *syn_while_do_parse( SynState *ss, SynNode *parent )
{
	SynNode *node, *node1, *node2;
	node = syn_create_node( DOMAIN, TK_WHILE, ss );
	node->parent = parent;
	syn_token_next( ss );
	syn_insert_lchild( node, syn_exp_parse( ss, node ) );
	if( syn_token_type( ss ) != TK_DO )
		;//error
	else
	{
		syn_token_next( ss );
		syn_insert_rchild( node, syn_domain_parse( ss, node ) );
		if( syn_token_type( ss ) != TK_OD )
			;//error
		syn_token_next( ss );
	}
	return node;
}

//static int compare_sym_type( Symbol *s1, Symbol *s2 )

static SynNode *syn_func_call_parse( SynState *ss, SynNode *parent )
{
	SynNode *node = 0, *fnode, *plist;
	Symbol *sym = syn_sym_qurry( parent, syn_token_value( ss ) );
	fnode = (SynNode*)sym->func_info;
	plist = fnode->lchild;
	syn_token_next( ss );
	int ty = syn_token_type( ss );
	if( ty != '(' )
		;//error

	node = syn_create_node( EXP, FUNC_CALL, ss );
	while( plist )
	{
		syn_token_next( ss );
	    ty = syn_token_type( ss );
	    if( TK_ID == ty )
	    {
	    	Symbol *tmp = syn_sym_qurry( parent, ss->pos->token.value );
	    	if( plist->node_sym.type == tmp->type )
	    		;//
	    	else
	    		;//error
	    }
	    plist = plist->sibling;
	    if( plist == 0 )
	    {
	    	syn_token_next( ss );
	    	ty = syn_token_type( ss );
	    	if( ty != ')' )
	    		;//error
	    }
	    else
	    {
			syn_token_next( ss );
		    ty = syn_token_type( ss );
		    if( ty != ',' )
		    	;//error
	    }
	}
	return node;
}
static SynNode *syn_exp_parse( SynState *ss, SynNode *parent )
{
	SynNode *node = 0;
	Symbol *tmp;
	int ty = syn_token_type( ss );
	int index;
	switch( ty )
	{
	case TK_ID: //MISS FUNC CALL
		tmp = syn_sym_qurry( parent, syn_token_value( ss ) );
		if( 0 == tmp )
			;//error
		node = syn_create_node( EXP, ID, ss );
		node->rsymbol = tmp;
	case TK_CONST_VALUE:
		if( ty != TK_ID )
		{
			node = syn_create_node( EXP, CONST, ss );
			node->parent = parent;
			Symbol *tmp = syn_sym_qurry( node, syn_token_value( ss ) );
			if( 0 == tmp )
				node->rsymbol = syn_update_sym_table( node, 0, syn_token_subtype( ss ), ss->pos->token.value );
			else node->rsymbol = tmp;
		}
		syn_token_next( ss );
	    ty = syn_token_type( ss );
		if( operator_check( ty ) )
		{
			SynNode *node2 = syn_create_node( EXP, ty, ss );
			syn_insert_lchild( node2, node );
			syn_token_next( ss );
			syn_insert_rchild( node2, syn_exp_parse( ss, parent ) );
			return syn_adjust_prior( node2 );
		}
		else if( '=' == ty )
		{
			SynNode *node2 = syn_create_node( STMT, ASSIGN, ss );
			syn_insert_lchild( node2, node );
			syn_token_next( ss );
			syn_insert_rchild( node2, syn_exp_parse( ss, parent ) );
			return node2;
		}
		return node;
	case '(':
		node = syn_create_node( EXP, '(', ss );
		syn_token_next( ss );
		node->lchild = syn_exp_parse( ss, parent );
		if( syn_token_type( ss ) == ')' )
		{
			syn_token_next( ss );
			ty = syn_token_type( ss );
			if( operator_check( ty ) )
			{
				SynNode *node2 = syn_create_node( EXP, ty, ss );
				node2->lchild = node;
				syn_token_next( ss );
				node2->rchild = syn_exp_parse( ss, parent );
				SynNode *tmpNode = syn_adjust_prior( node2 );
				if( tmpNode != node2 )
					if( tmpNode->lchild )
						tmpNode->lchild = syn_adjust_prior( tmpNode->lchild );
				return tmpNode;
			}
			return node;
		}
		else
			;//error
	case ')':
		return 0;
	// FUNC CALL
	}
	return 0;
}

int operator_check( int type )
{
	if( '+' == type || '-' == type || '*' == type || '/' == type || '%' == type ||\
			TK_EQ == type || TK_GE == type || TK_LE == type || TK_NE == type ||\
			TK_AND == type || TK_OR == type || '>' == type || '<' == type )
		return 1;
	return 0;
}

static void syn_node_print( SynNode *node, int step) /*for debug*/
{
	while( /*(node = node->sibling)*/node )
	{
		int i = 0;
		for( ; i< step; i++) printf("---");
		if( step > 0 )
			if( node->parent )
				if( node == node->parent->lchild ) printf( "L" );
			    else printf( "R" );
		printf("s%d: %s   ", step, node->node_sym.value.str_val);
		if( node->type == DOMAIN )
		{
			printf( "domain " );
			Symbol * sb = node->sym_table;
			while( sb ) {printf( "%s:%s ", sb->name, sb->value.str_val ); sb = sb->next;}
		}
		printf("\n");
		//fflush(stdout);
		if( node->lchild )
			syn_node_print(node->lchild, step + 1 );
		if( node->rchild )
			syn_node_print(node->rchild, step + 1 );
		//printf("sibling->");
		node = node->sibling;
		//syn_node_print( node, step );
	}
}

static SynNode *syn_domain_parse( SynState *ss, SynNode *parent )
{
	SynNode *root = 0, *pre = 0, *node, *tmp;
	int ty = syn_token_type( ss );
	while( TK_ERROR != ty && TK_EOF != ty && 0 != ty && ss->pos )
	{
		node = 0;
		switch( ty )
		{
		case TK_ID:
			if( 0 ) /* def test */
				break;
			if( 0 ) /* func call */
				break;
			Symbol *tmp_sym = syn_sym_qurry( parent, syn_token_value( ss ) );
			if( tmp_sym )
			{
				if( FUNC != tmp_sym->type )
				{
					node = syn_exp_parse( ss, parent );
					node->parent = parent;
				}
				else
				{
					//func call
				}
				break;
			}
		case Tk_CONST:
//		case TK_INT:
//		case TK_LONG:
//		case TK_LONGLONG:
//		case TK_UINT:
//		case TK_ULONG:
//		case TK_ULONGLONG:
//		case TK_SHORT:
//		case TK_USHORT:
//		case TK_FLOAT:
		case TK_NUMBER:
		case TK_CHAR:
			node = syn_id_def_parse( ss, parent, 0 , syn_token_type( ss ), 0 );
			node->parent = parent;
			tmp = node;
			/*while( tmp ) /* add id_def to symbol table del?
			{
				if( tmp->subType.stmt == ASSIGN )
				{
					if( !tmp->rchild )
						syn_update_sym_table( tmp, tmp->lchild->value.str_val, tmp->lchild->type, 0 );
					else
						syn_update_sym_table( tmp, tmp->lchild->value.str_val, tmp->type, tmp->rchild->value.str_val );
				}
				else
				    syn_update_sym_table( tmp, tmp->value.str_val, tmp->type, 0 );
				tmp = tmp->sibling;
			}*/
			break;
		case TK_FUNCTION:
			node = syn_func_def_parse( ss, parent );
			node->parent = parent;
			tmp = node;
			/*while( tmp ) /* add param_def of function to symbol table
			{
				if( tmp->type == DOMAIN )
					syn_update_sym_table( tmp->parent, tmp->value.str_val, tmp->type, 0 );
				else if( tmp->subType.stmt == ASSIGN )
				{
					if( !tmp->rchild )
						syn_update_sym_table( tmp, tmp->lchild->value.str_val, tmp->lchild->type, 0 );
					else
						syn_update_sym_table( tmp, tmp->lchild->value.str_val, tmp->type, tmp->rchild->value.str_val );
				}
				else
				    syn_update_sym_table( tmp, tmp->value.str_val, tmp->type, 0 );
				tmp = tmp->sibling;
			}*/
			//syn_token_next( ss );
			break;
		case TK_IF:
			node = syn_if_parse( ss, parent, TK_IF );
			//syn_token_next( ss );
			break;
		case TK_ELIF:
			if( pre && ( TK_IF == pre->subType.exp || TK_ELIF == pre->subType.exp ) )
				node = syn_if_parse( ss, parent, TK_ELIF );
			else
				;//error
			break;
		case TK_ELSE:
			if( pre && ( TK_IF == pre->subType.exp || TK_ELIF == pre->subType.exp ) )
				node = syn_if_parse( ss, parent, TK_ELSE );
			else
				;//error
			break;
		case TK_WHILE:
			node = syn_while_do_parse( ss, parent );
			//syn_token_next( ss );
			break;
		case TK_BREAK:
			node = syn_create_node( STMT, TK_BREAK, ss );
			syn_token_next( ss );
		case TK_CONTINUE:
			node = syn_create_node( STMT, TK_CONTINUE, ss );
			syn_token_next( ss );
		case TK_DO:
			tmp = syn_create_node( DOMAIN, -1, ss );
			tmp->parent = parent;
			syn_token_next( ss );
			node = syn_domain_parse( ss, tmp );
			node->parent = tmp;
			syn_insert_lchild( tmp, node );
			node = tmp;
			if( syn_token_type( ss ) != TK_OD )
				 ;//ERROR
			syn_token_next( ss );
			break;
		case TK_OD:
			return root;
		case ';':
			syn_token_next( ss );
		    break;
		default:
			syn_token_next( ss );
		}
		if( ss->pos ) ty = syn_token_type( ss );
		if( !node ) continue;
		if( root == 0 )
		{
			root = node;
			while( node->sibling ) node = node->sibling;
			pre = node;
		}
		else
		{
			pre->sibling = node;
			while( node->sibling ) node = node->sibling;
			pre = node;
		}
	}
	return root;
}

SynNode *syn_parse( SynState *ss )
{
	global_id_num = 0;
	SynNode *node = syn_create_node( DOMAIN, GLOBAL, ss );
	syn_insert_rchild( node, syn_domain_parse( ss, node ) );
	syn_node_print( node, 0 );
	return node;
}
