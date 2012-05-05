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
static int create_domain_bytecode( State *s, SynNode *sn, VM *vm, int w );
static int get_function( char *fname, VM *vm );

State *create_bytecode( SynNode *sn, char *name, VM *vm )
{
	State *s = (State*)malloc( sizeof( State ) );
	vm->__global_state_table[state_index++] = s;

	s->name = (char*)malloc( strlen( name ) );
	strcpy( s->name, name );
	s->code = (int*)malloc(DEFAULT_MEM_LEN*sizeof(int));
	s->clen = 0;
	//s->offset = (vm->sp - vm->bp)/sizeof(SymValue);//cac it now?!
	s->soffset = 0;
	s->kcnt = 0;
	SynNode *p = sn->lchild;
	s->pnum = 0;
	int cnt = 0;
	while( p )
	{
		//s->param[s->pnum] = p->rnum;
		p->offset = -cnt;
		s->pnum++;
		cnt++;
		p = p->sibling;
	}

	create_domain_bytecode( s, sn->rchild, vm, -1 );
	return s;
}

static int get_function( char *fname, VM *vm )
{
	int i;
	for( i = 0; i < state_index; i++ )
		if( strcmp( fname, vm->__global_state_table[i]->name ) == 0 )
			return i;
	return -1;
}
static int const_value_check( State *s, SymValue v )
{
	int i = 0;
	for( ; i < s->kcnt; i++ )
	{
		if( s->K[i].unit_size != v.unit_size )
			continue;
		if( s->K[i].len != v.len )
			continue;
		if( strncmp( s->K[i].addr, v.addr, v.len*v.unit_size ) == 0 )
			return i;
	}
	return -1;
}

static int create_domain_bytecode( State *s, SynNode *sn, VM *vm, int w ) // w is for 'continue' in loop
{
	Symbol *tmp = sn->parent->sym_table;
	while( tmp ) //update const symbol table
	{
		if( 0 == tmp->name && tmp->type == STRING )
		{
			if( const_value_check( s, tmp->value ) >= 0 )
				continue;
			s->K[s->kcnt] = tmp->value;
			if( STRING == tmp->type )
			{
				s->K[s->kcnt].unit_size = tmp->value.unit_size;
				s->K[s->kcnt].len = tmp->value.len;
				int msize = tmp->value.unit_size*tmp->value.len;
				s->K[s->kcnt].addr = malloc( msize );
				memcpy( s->K[s->kcnt].addr, tmp->value.addr, msize );
			}
			tmp->offset = s->kcnt;
			s->kcnt++;
		}
		tmp = tmp->next;
	}
	tmp = sn->parent->sym_table;
	int push_count = 0;
	while( tmp ) //push local vars
	{
		if( tmp->name == 0 )
		{
			tmp = tmp->next;
			continue;
		}
		tmp->offset = s->soffset++;
		if( NUMBER == tmp->type )
		{
			s->code[s->clen] = CREATE_IAx( OP_MOVEI, DX ); s->clen += 3;
			yl_number *yltmp = (yl_number*)(&s->code[s->clen-2]);
			*yltmp = tmp->value._number;
		}
		s->code[s->clen] = CREATE_IAx( OP_PUSH, DX ); s->clen = s->clen + 1;
		tmp = tmp->next;
		push_count++;
	}
	int break_test = -1;
	while( sn )
	{
		if( sn->subType.stmt == ASSIGN )
		{
			int r = create_exp_bytecode( s, sn->rchild );
			s->code[s->clen] = CREATE_IAB( OP_MOVE, sn->lchild->rsymbol->offset, r );
			s->clen = s->clen + 1;
		}
		else if( sn->subType.stmt == TK_IF )
		{
			int mark[MAX_IF_BRANCH_NUM];
			int if_num = 0;
			create_exp_bytecode( s, sn->lchild );
			int ctmp = s->clen;
			s->clen = s->clen + 1;
			//create domain bytecode
			create_domain_bytecode( s, sn->rchild, vm, w );
			mark[if_num] = s->clen; if_num++;
			s->clen += 1;
			s->code[ctmp] = CREATE_IAx( OP_JNE, s->clen );

			while( sn )
			{
				if( TK_ELIF == sn->sibling->subType.stmt )
					sn = sn->sibling;
				else
					break;
				create_exp_bytecode( s, sn->lchild );
				int ctmp = s->clen;
				s->clen = s->clen + 1;
				//create domain bytecode
				create_domain_bytecode( s, sn->rchild, vm, w );
				mark[if_num] = s->clen; if_num++;
				s->clen += 1;
				s->code[ctmp] = CREATE_IAx( OP_JNE, s->clen );
			}
			if( sn )
			if( sn->sibling )
			if( TK_ELSE == sn->sibling->subType.stmt )
			{
				sn = sn->sibling;
				create_domain_bytecode( s, sn->rchild, vm, w );
			}
			int i;
			for( i = 0; i < if_num; i++ )
				s->code[mark[i]] = CREATE_IAx( OP_JMP, s->clen );
		}
		else if( sn->subType.stmt == TK_WHILE )
		{
			int ctmp1 = s->clen;
			create_exp_bytecode( s, sn->lchild );
			int ctmp2 = s->clen;
			s->clen = s->clen + 1;
			//create domain bytecode
			create_domain_bytecode( s, sn->rchild, vm, ctmp1 );
			s->code[s->clen] = CREATE_IAx( OP_JMP, ctmp1 ); s->clen += 1;
			s->code[ctmp2] = CREATE_IAx( OP_JNE, s->clen );
		}
		else if( sn->subType.stmt == TK_BREAK )
		{
			break_test = s->clen;
			s->clen += 1;
		}
		else if( sn->subType.stmt == TK_CONTINUE )
		{
			s->code[s->clen] = CREATE_IAx( OP_JMP, w );
			s->clen += 1;
		}
		else if( sn->subType.stmt == TK_FUNCTION ) // function declear
		{
			create_bytecode( sn, 0, vm );
		}
		else if( sn->subType.stmt == 'TK_FUNC_CALL' )
		{
			int fnum = get_function( sn->node_sym.value.str_val, vm );
			State *fs = vm->__global_state_table[fnum];
			s->code[s->clen] = CREATE_IAx( OP_PUSH, IP ); s->clen = s->clen + 1;
			s->code[s->clen] = CREATE_IAx( OP_PUSH, CX ); s->clen = s->clen + 1;
			s->code[s->clen] = CREATE_IAx( OP_PUSH, BX ); s->clen = s->clen + 1;
			SynNode *tmp = sn->lchild;
			while( tmp )
			{
				s->code[s->clen] = CREATE_IAx( OP_PUSH, tmp->rnum ); s->clen = s->clen + 1;
				tmp = tmp->sibling;
			}
			s->code[s->clen] = CREATE_IAx( OP_CALL, fnum ); s->clen = s->clen + 1;
			s->code[s->clen] = CREATE_IAx( OP_POP, BX ); s->clen = s->clen + 1;
			s->code[s->clen] = CREATE_IAx( OP_POP, CX ); s->clen = s->clen + 1;
			s->code[s->clen] = CREATE_IAx( OP_POP, IP ); s->clen = s->clen + 1;
		}
		sn = sn->sibling;
	}
	if( break_test > 0 )
		s->code[break_test] = CREATE_IAx( OP_JMP, s->clen );
	while( push_count ) //pop local vars
	{
		//if( tmp->name == 0 )
		//	continue;
		//tmp->offset = s->soffset--;
		s->code[s->clen] = CREATE_IAx( OP_POP, 0 ); s->clen = s->clen + 1;
		//tmp = tmp->next;
		push_count--;
	}
	return 0;
}

static int create_assign_bytecode( State *s, SynNode *sn ) // for what?
{
	if( sn->type != '=' )
	{
		int ty = sn->type;
		if(sn->type == TK_ID)
			return sn->rnum;
		else if( '+' == ty || '-' == ty || '*' == ty || '/' == ty || '%' == ty )
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
	int ty = sn->subType.exp;
	if( operator_check( ty ) )
	{
		create_exp_bytecode( s, sn->lchild );
		s->code[s->clen] = CREATE_IAx( OP_PUSH, AX );
		s->clen = s->clen + 1;
		create_exp_bytecode( s, sn->rchild );
		s->code[s->clen] = CREATE_IAx( OP_POP, DX );
		s->clen = s->clen + 1;

		switch( ty )
		{
		case '+':
			s->code[s->clen] = CREATE_IAB( OP_ADD, DX, AX );
			s->clen = s->clen + 1;
		    break;
		case '-':
			s->code[s->clen] = CREATE_IAB( OP_SUB, DX, AX );
			s->clen = s->clen + 1;
		    break;
		case '*':
			s->code[s->clen] = CREATE_IAB( OP_MUL, DX, AX );
			s->clen = s->clen + 1;
			break;
		case '/':
			s->code[s->clen] = CREATE_IAB( OP_DIV, DX, AX );
			s->clen = s->clen + 1;
			break;
		case '%':
			s->code[s->clen] = CREATE_IAB( OP_MOD, DX, AX );
			s->clen = s->clen + 1;
			break;
		case TK_EQ:
			s->code[s->clen] = CREATE_IAB( OP_EQ, DX, AX );
			s->clen = s->clen + 1;
			break;
		case TK_GE:
			s->code[s->clen] = CREATE_IAB( OP_GE, DX, AX );
			s->clen = s->clen + 1;
			break;
		case TK_LE:
			s->code[s->clen] = CREATE_IAB( OP_LE, DX, AX );
			s->clen = s->clen + 1;
			break;
		case TK_NE:
			s->code[s->clen] = CREATE_IAB( OP_NE, DX, AX );
			s->clen = s->clen + 1;
			break;
		case TK_OR:
			s->code[s->clen] = CREATE_IAB( OP_OR, DX, AX );
			s->clen = s->clen + 1;
			break;
		case TK_AND:
			s->code[s->clen] = CREATE_IAB( OP_AND, DX, AX );
			s->clen = s->clen + 1;
			break;
		case '>':
			s->code[s->clen] = CREATE_IAB( OP_G, DX, AX );
			s->clen = s->clen + 1;
			break;
		case '<':
			s->code[s->clen] = CREATE_IAB( OP_L, DX, AX );
			s->clen = s->clen + 1;
			break;
		}
	}
	else if( '(' == ty )
	{
		return create_exp_bytecode( s, sn->lchild );
	}
	else //if( sn->subType.stmt == CONST )
	{
		if( 0 == sn->rsymbol->name && sn->type == NUMBER )//immediate
		{
			s->code[s->clen] = CREATE_IAx( OP_MOVEI, AX ); s->clen += 1;
			//! test
			int test = GET_OPCODE(s->code[s->clen-1]);
			yl_number *yltmp = (yl_number*)(&s->code[s->clen]);
			s->clen += 2;
			*yltmp = sn->rsymbol->value._number;
			test = GET_OPCODE(s->code[s->clen-3]);
			test = 0;
		}
		else
		{
			s->code[s->clen] = CREATE_IAB( OP_MOVE, AX, sn->rsymbol->offset );
			s->clen = s->clen + 1;
		}
	}
	return AX;
}

static get_opvalue_a( VM *vm, State *s, int ra )
{
	SymValue *sa;
	if( ra >= AX ) sa = ((SymValue*)vm) + IP - ra;
	else sa = ((SymValue*)s->sbp) + ra;
	return sa;
}
static get_opvalue_b( VM *vm, State *s, int rb )
{
	SymValue *sb;
	if( rb >= AX ) sb = ((SymValue*)vm) + IP - rb;
	else sb = ((SymValue*)s->sbp) + rb;
	return sb;
}

void execute( VM *vm )
{
	State *s = vm->__global_state_table[0];
	vm->ip.int_val = -1;
	s->sbp = vm->sp;
	short ra, rb, vsize = sizeof( SymValue );
	SymValue *sa, *sb;
	while( vm->ip.int_val < s->clen - 1 )
	{
		vm->ip.int_val += 1;
		size_t opcode = GET_OPCODE( s->code[vm->ip.int_val] );
		switch( opcode )
		{
		case OP_MOVE:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_b( vm, s, rb );
			*sa = *sb;
			continue;
		case OP_MOVEI:
			ra = GET_ARG_Ax(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			vm->ip.int_val++;
			sb = (SymValue*)(&s->code[vm->ip.int_val]);
			*sa = *sb;
			vm->ip.int_val++;
			continue;
		case OP_JMP:
			ra = GET_ARG_Ax(s->code[vm->ip.int_val]);
			vm->ip.int_val = ra - 1;
			continue;
		case OP_JNE:
			ra = GET_ARG_Ax(s->code[vm->ip.int_val]);
			if( vm->ax.int_val == 0 )
				vm->ip.int_val = ra - 1;
			continue;
		case OP_JE:
			ra = GET_ARG_Ax(s->code[vm->ip.int_val]);
			if( vm->ax.int_val )
				vm->ip.int_val = ra - 1;
			continue;
		case OP_ADD:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax._number = sa->_number + sb->_number;
			continue;
		case OP_SUB:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax._number = sa->_number - sb->_number;
			continue;
		case OP_MUL:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax._number = sa->_number * sb->_number;
			continue;
		case OP_DIV:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax._number = sa->_number / sb->_number;
			continue;
		case OP_MOD:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax._number = sa->ll_val - sa->ll_val / sb->ll_val;
			continue;
		case OP_EQ:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax.int_val = 0;
			if( sa->ll_val == sb->ll_val )
				vm->ax.int_val = 1;
			continue;
		case OP_GE:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax.int_val = 0;
			if( sa->ll_val >= sb->ll_val )
				vm->ax.int_val = 1;
			continue;
		case OP_LE:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax.int_val = 0;
			if( sa->ll_val <= sb->ll_val )
				vm->ax.int_val = 1;
			continue;
		case OP_NE:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax.int_val = 0;
			if( sa->ll_val != sb->ll_val )
				vm->ax.int_val = 1;
			continue;
		case OP_G:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax.int_val = 0;
			if( sa->ll_val > sb->ll_val )
				vm->ax.int_val = 1;
			continue;
		case OP_L:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax.int_val = 0;
			if( sa->ll_val < sb->ll_val )
				vm->ax.int_val = 1;
			continue;
		case OP_OR:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax.int_val = 0;
			if( sa->ll_val || sb->ll_val )
				vm->ax.int_val = 1;
			continue;
		case OP_AND:
			ra = GET_ARG_A(s->code[vm->ip.int_val]);
			rb = GET_ARG_B(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			sb = get_opvalue_a( vm, s, rb );
			vm->ax.int_val = 0;
			if( sa->ll_val && sb->ll_val )
				vm->ax.int_val = 1;
			continue;
		case OP_PUSH:
			ra = GET_ARG_Ax(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			//memcpy( vm->sp, sa, vsize );
			sb = (SymValue*)(vm->sp);
			*sb = *sa;
			vm->sp += vsize;
			continue;
		case OP_POP:
			ra = GET_ARG_Ax(s->code[vm->ip.int_val]);
			sa = get_opvalue_a( vm, s, ra );
			vm->sp -= vsize;
			sb = (SymValue*)(vm->sp);
			*sa = *sb;
			continue;
		}
	}
}

VM* create_vm()
{
	VM *vm = (VM*)malloc( sizeof( VM ) );
	vm->bp = (char*)malloc( DEFAULT_STACK_SIZE );
	vm->sp = vm->bp;
	vm->ip._number = 0;
	int i;
	for( i = 0; i < MAX_STATE_NUM; i++ )
		vm->__global_state_table[i] = 0;
	return vm;
}
