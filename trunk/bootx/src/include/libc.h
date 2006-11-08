/* 
 *   Creation Date: <2002/07/20 13:05:15 samuel>
 *   Time-stamp: <2002/07/23 13:27:33 samuel>
 *   
 *	<libc.h>
 *	
 *	Minimal set of headers
 *   
 *   Copyright (C) 2002 Samuel Rydh (samuel@ibrium.se)
 *
 */

#ifndef _H_LIBC
#define _H_LIBC

#include <sys/types.h>
#include <byteswap.h>

#define printf	printm
extern int	printm( const char *fmt, ... );

#include "libclite.h"

#ifndef __P		/* prototype support */
#define	__P(x)	x
#endif

#ifndef bswap16
#define bswap16	bswap_16
#define bswap32	bswap_32
#endif

#endif   /* _H_LIBC */
