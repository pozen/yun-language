/**
 * pozen@yl:~>file 'yun_vm.h'
 * pozen@yl:~>brief 'vm'
 * pozen@yl:~>date 'Oct 15, 2011'
 * pozen@yl:~>author 'pozen'
 */

#ifndef __YUN_VM_H_
#define __YUN_VM_H_

//#include <stdio.h>
#include "yun_symtable.h"

typedef enum{
 iAB, iAx, isAx
}iType;

#define SIZE_OP 6
#define SIZE_A  13
#define SIZE_B  13
#define SIZE_Ax ( SIZE_B + SIZE_A )

#define POS_OP 0
#define POS_A  ( POS_OP + SIZE_OP )
#define POS_B  ( POS_A + SIZE_A )
#define POS_Ax POS_A

#define MASK1( n, p ) ( (~( ( ~0 ) << n ) ) << p )
#define MASK0( n, p ) ( ~MASK1( n, p ) )

#define GET_OPCODE( i ) ( ( ( i ) >> POS_OP ) & MASK1( SIZE_OP, 0 ) )
#define SET_OPCODE( i, op ) ( ( i ) = ( ( i ) & MASK0( SIZE_OP, POS_OP ) ) | ( op << POS_OP ) )

#define GET_ARG_A( i ) ( ( ( i ) >> POS_A ) & MASK1( SIZE_A, 0 ) )
#define SET_ARG_A( i, a ) ( ( i ) = ( ( i ) & MASK0( SIZE_A, POS_A ) ) | ( a << POS_A ) )

#define GET_ARG_B( i ) ( ( ( i ) >> POS_B ) & MASK1( SIZE_B, 0 ) )
#define SET_ARG_B( i, a ) ( ( i ) = ( ( i ) & MASK0( SIZE_B, POS_B ) ) | ( a << POS_B ) )

#define GET_ARG_Ax( i ) ( ( ( i ) >> POS_Ax ) & MASK1( SIZE_Ax, 0 ) )
#define SET_ARG_Ax( i, a ) ( ( i ) = ( ( i ) & MASK0( SIZE_Ax, POS_Ax ) ) | ( a << POS_Ax ) )

#define CREATE_IAB( op, a, b ) ( ( ( op ) << POS_OP ) | ( ( a ) << POS_A ) | ( ( b ) << POS_B ) )
#define CREATE_IAx( op, a ) ( ( ( op ) << POS_OP ) | ( ( a ) << POS_Ax ) )

typedef enum {
	OP_MOVE,
	OP_MOVEI,
	OP_MOVES,
	OP_LOAD,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_POW,
	OP_UNM,
	OP_NOT,

	OP_JMP,
	OP_JE,
	OP_JNE,
	OP_EQ,
	OP_LT,
	OP_LE,
	OP_GE,
	OP_NE,
	OP_G,
	OP_L,
	OP_OR,
	OP_AND,

	OP_CALL,
	OP_RETURN,
	OP_PUSH,
	OP_POP
} OpCode;

#define DEFAULT_RSIZE 256
#define MAX_PARAM_NUM 16

typedef struct State {
	char *name;
	//Symbol R[DEFAULT_RSIZE*2];
	//size_t *S[DEFAULT_RSIZE*2];
	SymValue K[DEFAULT_RSIZE];
	int clen;
	int *code;
	size_t mlen;
	size_t pnum;
	size_t param[MAX_PARAM_NUM];
	char* sbp;
	int soffset;
	size_t kcnt;
	struct State *next;//?
} State;

#define DEFAULT_MEM_LEN 8096
#define MAX_STATE_NUM 2048
#define MAX_IF_BRANCH_NUM 100

typedef struct {
	size_t snum;
	size_t rvalue;
}FuncCall;

typedef struct {
	SymValue ip;
	SymValue dx;
	SymValue cx;
	SymValue bx;
	SymValue ax;
	//int if;
    char *sp;
	char *bp;
	char *hp;
	char *shp;
	int stacksize;
	FuncCall fc;
	State *__global_state_table[MAX_STATE_NUM];
} VM;

struct SynNode;
#define REGISTER_NUM 5
#define AX 8187
#define BX 8188
#define CX 8189
#define DX 8190
#define IP 8191

#define DEFAULT_STACK_SIZE 1024*1024

struct SynNode;
State *create_bytecode( struct SynNode *sn, char *name, VM *vm );
State *load_bytecode( char *filename );
VM* create_vm();
int save_bytecode( char *filename, State* s );
void execute( VM *vm );

#endif /* __YUN_VM_H_ */
