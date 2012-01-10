/**
 * pozen@yl:~>file 'yun_vm.c'
 * pozen@yl:~>brief 'vm'
 * pozen@yl:~>date 'Oct 22, 2011'
 * pozen@yl:~>author 'pozen'
 */

#include "yun_vm.h"
#include "yun_parser.h"

static int state_index = 0;

static int create_assign_bytecode( State *s, SynNode *sn );
static int create_exp_bytecode( State *s, SynNode *sn );
static int create_domain_bytecode( State *s, SynNode *sn );
static int get_function(char *fname);

State *create_bytecode( SynNode *sn, char *name )
{
	State *s = (State*)malloc( sizeof( State ) );
	__global_state_table[state_index++] = s;

	s->name = (char*)malloc( strlen( name ) );
	strcpy( s->name, name );
	s->code = (int*)malloc(DEFAULT_MEM_LEN*sizeof(int));
	int i = 0;
	Symbol *tmp = sn->sym_table;
	while( tmp )
	{
		s->R[i] = (void*)malloc( sizeof( SymValue ) );
		*s->R[i] = tmp->value;
		tmp = tmp->next;
		i++;
	}
	s->clen = 0;
	create_domain_bytecode( s, sn );
	return s;
}

static int get_function(char *fname)
{
	for( int i = 0; i < state_index; i++ )
		if( strcmp( fname, __global_state_table[i]->name ) == 0 )
			return i;
	return -1;
}

static int create_domain_bytecode( State *s, SynNode *sn )
{
	while( sn )
	{
		if( sn->type == '=' )
		{
			s->code[s->clen] = CREATE_IAB( OP_MOVE, sn->lchild->rnum, create_assign_bytecode( s, sn->rchild ) );
			s->clen = s->clen + 1;
		}
		else if( sn->type == 'TK_IF' )
		{
			create_exp_bytecode( s, sn->rchild );
			int ctmp = s->clen;
			s->clen = s->clen + 1;
			//create domain bytecode
			create_domain_bytecode(s, sn->lchild);
			code[ctmp] = CREATE_IAx( OP_JNE, s->clen - ctmp + 1 );
		}
		else if( sn->type == 'TK_WHILE' )
		{
			int ctmp = s->clen + 1;
			create_exp_bytecode( s, sn->rchild );
			//create domain bytecode
			code[ctmp] = CREATE_IAx( OP_JNE, ctmp - s->clen );
		}
		else if( sn->type == 'TK_FUNCTION' ) // function declear
		{
			create_bytecode( sn, 0 );
		}
		else if( sn->type == 'TK_FUNC_CALL' )
		{
			int fnum = get_function(  );
			code[clen] = CREATE_IAx( OP_PUSH, IP ); s->clen = s->clen + 1;
			code[clen] = CREATE_IAx( OP_PUSH, CX ); s->clen = s->clen + 1;
			code[clen] = CREATE_IAx( OP_PUSH, BX ); s->clen = s->clen + 1;
			code[clen] = CREATE_IAx( OP_PUSH, ARG1 );//...s->clen = s->clen + 1;
			code[clen] = CREATE_IAx( OP_CALL, ADDR ); s->clen = s->clen + 1;
			code[clen] = CREATE_IAx( OP_POP, BX ); s->clen = s->clen + 1;
			code[clen] = CREATE_IAx( OP_POP, CX ); s->clen = s->clen + 1;
			code[clen] = CREATE_IAx( OP_POP, IP ); s->clen = s->clen + 1;
		}
		sn = sn->sibling;
	}
	return 0;
}

static int create_assign_bytecode( State *s, SynNode *sn )
{
	if( sn->type != '=' )
	{
		if(sn->type == TK_ID)
			return sn->rnum;
		else if( sn->type == '+'/* op */ )
		{
			create_exp_bytecode( s, sn );
			return AX;
		}
		else
			;//error
	}
	s->code[s->clen] = CREATE_IAB( OP_MOVE, sn->lchild->rnum, create_assign_bytecode( s, sn->rchild ));
	s->clen = s->clen + 1;
	return sn->lchild->rnum;
}

static int create_exp_bytecode( State *s, SynNode *sn )
{
	if( sn->type/*op*/ )
	{
		create_exp_bytecode( s, sn->lchild );
		s->code[s->clen] = CREATE_IAx( OP_PUSH, AX );
		s->clen = s->clen + 1;
		create_exp_bytecode( s, sn->rchild );
		s->code[s->clen] = CREATE_IAx( OP_PUSH, AX );
		s->clen = s->clen + 1;
		s->code[s->clen] = CREATE_IAx( OP_POP, DX );
		s->clen = s->clen + 1;
		s->code[s->clen] = CREATE_IAx( OP_POP, AX );
		s->clen = s->clen + 1;
		switch( sn->type )
		{
		case '+':
			s->code[s->clen] = CREATE_IAB( OP_ADD, AX, DX );
			s->clen = s->clen + 1;
		    break;
		case '-':
			s->code[s->clen] = CREATE_IAB( OP_SUB, AX, DX );
			s->clen = s->clen + 1;
		    break;
		case '*':
			s->code[s->clen] = CREATE_IAB( OP_MUL, AX, DX );
			s->clen = s->clen + 1;
			break;
		case '/':
			s->code[s->clen] = CREATE_IAB( OP_DIV, AX, DX );
			s->clen = s->clen + 1;
		case 'TK_OR':
			break;
		case 'TK_AND':
			break;
		}
	}
	else if( sn->type == TK_ID )
	{
		s->code[s->clen] = CREATE_IAB( OP_MOVE, AX, sn->rnum );
		s->clen = s->clen + 1;
	}
	return 0;
}
