/* 
 *   Creation Date: <2004/01/25 16:34:48 samuel>
 *   Time-stamp: <2004/01/25 21:45:34 samuel>
 *   
 *	<locks.h>
 *	
 *	
 *   
 *   Copyright (C) 2004 Samuel Rydh (samuel@ibrium.se)
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *   
 */

#ifndef _H_LOCKS
#define _H_LOCKS

#include <kern/simple_lock.h>

/************** mutex locks **************/

typedef mutex_t *mol_mutex_t;

static inline void init_MUTEX_mol( mol_mutex_t *mup ) {
	*mup = mutex_alloc( ETAP_NO_TRACE );
}

static inline void free_MUTEX_mol( mol_mutex_t *mup ) {
	mutex_free( *mup );
}

static inline void down_mol( mol_mutex_t *mup ) {
	mutex_lock( *mup );
}

static inline void up_mol( mol_mutex_t *mup ) {
	mutex_unlock( *mup );
}


/************** spinlocks **************/

typedef simple_lock_data_t mol_spinlock_t;

static inline void spin_lock_mol( mol_spinlock_t *lock ) {
	simple_lock( lock );
}
static inline void spin_unlock_mol( mol_spinlock_t *lock ) {
	simple_unlock( lock );
}
static inline void spin_lock_init_mol( mol_spinlock_t *lock ) {
	simple_lock_init( lock );
}


#endif   /* _H_LOCKS */
