/* 
 *   Creation Date: <2001/06/16 21:30:18 samuel>
 *   Time-stamp: <2004/02/24 00:40:53 samuel>
 *   
 *	<init.S>
 *	
 *	Asm glue for ELF images run inside MOL
 *   
 *   Copyright (C) 2001, 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *   
 */

#include "mol_config.h"
#include "processor.h"
#include "asmdefs.h"
#include "osi.h"

#define	STACK_SIZE	(1024*256)
.comm	stackspace,STACK_SIZE


/************************************************************************/
/*	Exception Vectors						*/
/************************************************************************/

#define VECTOR( v, dummy_str )		.org __vectors + v ; vector__##v
#define ILLEGAL_VECTOR( v, dummy )	.org __vectors + v ; bl trap_error ;

	.text
GLOBL(__vectors):
	.org 0x0
	
	.org 0x20
GLOBL(_start):
	lis	r1,(stackspace)@ha
	addi	r1,r1,(stackspace)@l
	bl 	entry

trap_error:
	mflr	r3
	b unexpected_excep

ILLEGAL_VECTOR( 0x100, "SystemReset" )
ILLEGAL_VECTOR( 0x200, "MachineCheck" )
ILLEGAL_VECTOR( 0x300, "DSI" )
ILLEGAL_VECTOR( 0x400, "ISI" )
ILLEGAL_VECTOR( 0x500, "IRQ" )
ILLEGAL_VECTOR( 0x600, "Alignment" )

VECTOR( 0x700, "Program" ):	/* skip over - used to probe illegal instructions */
	mfsrr0	r3
	addi	r3,r3,4
	mtsrr0	r3
	mfsrr1	r3		/* modify r3 */
	rfi

VECTOR( 0x800, "FPU Unavailable" ):
	mtsprg0	r3
	mfsrr1	r3
	ori	r3,r3,0x2000
	mtsrr1	r3
	mfsprg0	r3
	rfi

ILLEGAL_VECTOR( 0x900, "DEC" )
ILLEGAL_VECTOR( 0xa00, "Reserved" )
ILLEGAL_VECTOR( 0xb00, "Reserved" )

// r3=1 -> supervisor mode, r3=0 -> usermode
VECTOR( 0xc00, "Syscall" ):
	mtsprg0	r4
	mtsprg1	r5
	mfsrr1	r4
	li	r5,MSR_PR
	andc	r4,r4,r5
	cmpwi	r3,0
	beq	1f
	ori	r4,r4,MSR_PR	
1:	mtsrr1	r4
	mfsprg0	r4
	mfsprg1	r5
	rfi

ILLEGAL_VECTOR( 0xd00, "Trace" )
ILLEGAL_VECTOR( 0xe00, "FPU assist" )
ILLEGAL_VECTOR( 0xf00, "Performance monitor" )
ILLEGAL_VECTOR( 0xf20, "AltiVec" )
ILLEGAL_VECTOR( 0x1000, "Instruction TLB Miss - 603" )
ILLEGAL_VECTOR( 0x1100, "DataLoadTLBMiss - 603" )
ILLEGAL_VECTOR( 0x1200, "DataStoreTLBMiss - 603" )
ILLEGAL_VECTOR( 0x1300, "Instruction Breakpoint" )
ILLEGAL_VECTOR( 0x1400, "System Management Interrupt" )
ILLEGAL_VECTOR( 0x1500, "reserved" )
ILLEGAL_VECTOR( 0x1600, "AltiVec Assist" )
ILLEGAL_VECTOR( 0x1700, "Thermal Management Interrupt" )
	
GLOBL(__vectors_end):
