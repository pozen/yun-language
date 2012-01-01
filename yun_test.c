/*
 * yun_test.c
 *
 *  Created on: Oct 2, 2011
 *      Author: pozen
 */
#include "yun_lex.h"
#include "yun_parser.h"
#include <stdio.h>

int main()
{
	FILE *fp = fopen( "lex_test", "rb" );
	int size;
	char *buf;
	if( fp == 0 )
	{
		fprintf( stderr, "Cannot open %s\n", "lex_test" );
		return -1;
	}
	fseek( fp, 0, SEEK_END );
	size = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	buf = (char*) malloc( size + 1 );
	fread( buf, size, 1, fp );
	buf[size] = 0;
	fclose( fp );

	LexToken *tk = lex_get_token( buf );
	LexToken *tmp = tk;
	SynState *ss = (SynState*)malloc( sizeof( SynState ) );
	ss->pos = tk;

	/*while( tmp )
	{
		printf("line:%d type:%d value:%s\n", tmp->line_num, tmp->token.type, tmp->token.value);
		tmp = tmp->next;
	}*/
	syn_parse( ss );

	lex_destroy_token( tk );
	printf("end\n");
	return 0;
}
