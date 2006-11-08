/* 
 *   Creation Date: <2001/06/16 21:30:18 samuel>
 *   Time-stamp: <2002/07/20 02:17:13 samuel>
 *   
 *	<init.S>
 *	
 *	Asm glue for ELF images run inside MOL
 *   
 *   Copyright (C) 2001, 2002 Samuel Rydh (samuel@ibrium.se)
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *   
 */

	.section .bss
	.balign 32
	.space	1024*256		// 256 K stack
stack:	.space  64

#define	r1	1
#define	r2	2
#define	r3	3
#define	r4	4
#define	r5	5
#define	r6	6

#define GLOBL(lab)	; .globl lab ; lab

	
/************************************************************************/
/*	Exception Vectors						*/
/************************************************************************/

#define ILLEGAL_VECTOR( v )	.org v ; bl trap_error ;
	
	.text
GLOBL(__vectors):
	.org 0x0

	.org 0x20
GLOBL(_start):
	lis	r1,(stack)@ha
	addi	r1,r1,(stack)@l
	bl	entry
trap_error:
	mflr	r3
	b unexpected_excep

	ILLEGAL_VECTOR( 0x100 )
	ILLEGAL_VECTOR( 0x200 )
	ILLEGAL_VECTOR( 0x300 )
	ILLEGAL_VECTOR( 0x400 )
	ILLEGAL_VECTOR( 0x500 )
	ILLEGAL_VECTOR( 0x600 )
	ILLEGAL_VECTOR( 0x700 )

	.org 0x800
	mtsprg0	r3
	mfsrr1	r3
	ori	r3,r3,0x2000
	mtsrr1	r3
	mfsprg0	r3
	rfi

	ILLEGAL_VECTOR( 0x900 )
	ILLEGAL_VECTOR( 0xa00 )
	ILLEGAL_VECTOR( 0xb00 )
	ILLEGAL_VECTOR( 0xc00 )
	ILLEGAL_VECTOR( 0xd00 )
	ILLEGAL_VECTOR( 0xe00 )
	ILLEGAL_VECTOR( 0xf00 )
	ILLEGAL_VECTOR( 0xf20 )
	ILLEGAL_VECTOR( 0x1000 )
	ILLEGAL_VECTOR( 0x1100 )
	ILLEGAL_VECTOR( 0x1200 )
	ILLEGAL_VECTOR( 0x1300 )
	ILLEGAL_VECTOR( 0x1400 )
	ILLEGAL_VECTOR( 0x1500 )
	ILLEGAL_VECTOR( 0x1600 )
	ILLEGAL_VECTOR( 0x1700 )

GLOBL(__vectors_end):
