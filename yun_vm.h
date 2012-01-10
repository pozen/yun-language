/**
 * pozen@yl:~>file 'yun_vm.h'
 * pozen@yl:~>brief 'vm'
 * pozen@yl:~>date 'Oct 15, 2011'
 * pozen@yl:~>author 'pozen'
 */

#ifndef __YUN_VM_H_
#define __YUN_VM_H_

#include <stdio.h>

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

	OP_CALL,
	OP_RETURN,
	OP_PUSH,
	OP_POP
} OpCode;

#define DEFAULT_RSIZE 256

typedef struct State {
	char *name;
	void *R[DEFAULT_RSIZE];
	void *K[DEFAULT_RSIZE];
	size_t clen;
	int *code;
	size_t mlen;
	State *next;
} State;

typedef struct {
	int AX;
	int BX;
	int CX;
	int IP;
	int IF;

	int stacksize;
	int *stack;
	State *state;
} VM;

struct SynNode;

#define DEFAULT_MEM_LEN 8096
#define MAX_STATE_NUM 2048
State *__global_state_table[MAX_STATE_NUM];

State *create_bytecode( SynNode *sn, char *name );
State *load_bytecode( char *filename );
int save_bytecode( char *filename, State* s );
void execute( State *s );

#endif /* __YUN_VM_H_ */
