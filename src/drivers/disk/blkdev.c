/* 
 *   Creation Date: <2001/05/12 17:39:24 samuel>
 *   Time-stamp: <2004/03/24 01:34:29 samuel>
 *   
 *	<blkdev.c>
 *	
 *	Determine which block devices to export.
 *	The disks are claimed by lower level drivers.
 *   
 *   Copyright (C) 2001, 2002, 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *   
 */

#include "mol_config.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/ioctl.h>

#include "res_manager.h"
#include "partition_table.h"
#include "hfs_mdb.h"
#include "disk.h"
#include "llseek.h"
#include "booter.h"
#include "driver_mgr.h"

#define BLKFLSBUF  _IO(0x12,97)		/* from <linux/fs.h> */

/* Volumes smaller than this must be specified explicitely
 * (and not through for instance 'blkdev: /dev/hda -rw').
 * The purpose of this is preventing any boot-strap partitions
 * from beeing mounted (and hence deblessed) in MacOS.
 */
#define BOOTSTRAP_SIZE_MB	20

static opt_entry_t blk_opts[] = {
	{"-ro",			0 },
	{"-rw",			BF_ENABLE_WRITE },
	{"-force",		BF_FORCE },
	{"-cdrom",		BF_CD_ROM | BF_WHOLE_DISK | BF_REMOVABLE },
	{"-cd",			BF_CD_ROM | BF_WHOLE_DISK | BF_REMOVABLE },
	{"-cdboot",		BF_BOOT1 | BF_BOOT },
	{"-boot",		BF_BOOT },
	{"-boot1",		BF_BOOT1 | BF_BOOT },
	{"-whole",		BF_WHOLE_DISK },
	{"-removable",		BF_REMOVABLE },
	{"-drvdisk",		BF_DRV_DISK | BF_REMOVABLE },
	{"-ignore",		BF_IGNORE },
	{NULL, 0 }
};

enum { kMacVolumes, kLinuxDisks };

#define kDiskTypeUnknown	1
#define kDiskTypeHFSPlus	2
#define kDiskTypeHFS		4
#define kDiskTypeUFS		8
#define kDiskTypePartitioned	16

static bdev_desc_t 		*s_all_disks;


/************************************************************************/
/*	misc								*/
/************************************************************************/

/* volname is allocated */
static int
inspect_disk( int fd, char **type, char **volname )
{
	char buf[512];
	desc_map_t *dmap = (desc_map_t*)buf;
	hfs_mdb_t *mdb = (hfs_mdb_t*)buf;
	int signature;

	*type = "Disk";
	*volname = NULL;

	if( pread(fd, buf, 512, 0) != 512 )
		return kDiskTypeUnknown;
	
	if( dmap->sbSig == DESC_MAP_SIGNATURE ) {
		*volname = strdup("- Partioned -");
		return kDiskTypePartitioned;
	}

	if( pread(fd, buf, 512, 2*512) != 512 )
		return kDiskTypeUnknown;
	
	signature = hfs_get_ushort(mdb->drSigWord);

	if( signature == HFS_PLUS_SIGNATURE ) {
		*type = "Unembedded HFS+";
		return kDiskTypeHFSPlus;
	}

	if( signature == HFS_SIGNATURE ) {
		char vname[256];
		memcpy( vname, &mdb->drVN[1], mdb->drVN[0] );
		vname[mdb->drVN[0]] = 0;
		*volname = strdup( vname );

		if( hfs_get_ushort(mdb->drEmbedSigWord) == HFS_PLUS_SIGNATURE ) {
			*type = "HFS+";
			return kDiskTypeHFSPlus;
		}
		*type = "HFS";
		return kDiskTypeHFS;
	}
	return kDiskTypeUnknown;
}

/************************************************************************/
/*	register disk							*/
/************************************************************************/

static void
report_disk_export( bdev_desc_t *bd, const char *type )
{
	char buf[80];

	if( bd->flags & BF_DRV_DISK )
		return;

	strnpad( buf, bd->dev_name, 17 );
	if( buf[15] != ' ')
		buf[14] = buf[15] = '.';
	strncat3( buf, " ", bd->vol_name ? bd->vol_name : "", sizeof(buf));
	strnpad( buf, buf, 32 );

	printm("    %-5s %s <r%s ", type, buf, (bd->flags & BF_ENABLE_WRITE)? "w>" : "ead-only> ");
	
	if( bd->size )
		printm("%4ld MB ", (long)(bd->size >> 20) );
	else	
		printm(" ------ ");

	printm("%s%s\n", (bd->flags & BF_BOOT)? "BOOT" : "",
			 (bd->flags & BF_BOOT1)? "1" : "" );
}

static void
register_disk( const char *typestr, const char *name, const char *vol_name, int fd, long flags, ulong num_blocks )
{
	bdev_desc_t *bdev = malloc( sizeof(bdev_desc_t) );
	CLEAR( *bdev );

	bdev->dev_name = strdup(name);
	bdev->vol_name = vol_name ? strdup(vol_name) : NULL;

	bdev->fd = fd;
	bdev->flags = flags;
	bdev->size = 512ULL * num_blocks;

	bdev->priv_next = s_all_disks;
	s_all_disks = bdev;

	report_disk_export( bdev, typestr );
}


/************************************************************************/
/*     	mac disk support						*/
/************************************************************************/

static int
open_mac_disk( char *name, int flags, int constructed )
{
	int type, fd, ro_fallback, ret=0;
	char *volname, *typestr;
	uint size;
	
	if( (fd=disk_open(name, flags, &ro_fallback, constructed)) < 0 )
		return -1;
	type = inspect_disk( fd, &typestr, &volname );

	if( ro_fallback )
		flags &= ~BF_ENABLE_WRITE;

	if( type & (kDiskTypeHFSPlus | kDiskTypeHFS | kDiskTypeUFS) ) {
		/* standard mac volumes */
	} else if( type & kDiskTypePartitioned ) {
		if( constructed )
			ret = -1;
	} else {
		/* unknown disk type */
		if( !(flags & BF_FORCE) ) {
			if( !constructed )
				printm("----> %s is not a HFS disk (use -force to override)\n", name );
			ret = -1;
		}
	}
	size = get_file_size_in_blks(fd);

	/* detect boot-strap partitions by the small size */
	if( (flags & BF_ENABLE_WRITE) && (type & (kDiskTypeHFSPlus | kDiskTypeHFS)) ) {

		if( size/2048 < BOOTSTRAP_SIZE_MB && !(flags & BF_FORCE) ) {
			printm("----> %s might be a boot-strap partition.\n", name );
			ret = -1;
		}
	}

	if( !ret )
		register_disk( typestr, name, volname, fd, flags, get_file_size_in_blks(fd) );
	else
		close( fd );

	if( volname )
		free( volname );
	return ret;
}

static void
setup_mac_disk( char *name, int flags )
{
	static char *dev_list[] = { "/dev/sd", "/dev/hd", NULL };
	char buf[32], **pp;
	int i, n;
	
	/* /dev/hda type device? Substitute with /dev/hdaX */
	for( pp=dev_list; !(flags & BF_WHOLE_DISK) && *pp; pp++ ) {
		if( strncmp(*pp, name, strlen(*pp)) || strlen(name) != strlen(*pp)+1 )
			continue;

		flags |= BF_SUBDEVICE;
		flags &= ~BF_FORCE;
		for( n=0, i=1; i<32; i++ ) {
			sprintf( buf, "%s%d", name, i );
			n += !open_mac_disk( buf, flags, 1 );
		}
		if( !n )
			printm("No volumes found in '%s'\n", name );
		return;
	}
	open_mac_disk( name, flags, 0 );
}


/************************************************************************/
/*	setup disks							*/
/************************************************************************/

static void
setup_disks( char *res_name, int type )
{
	int fd, i, boot1, flags, ro_fallback=0;
	bdev_desc_t *bdev;
	char *name;
	
	for( i=0; (name=get_filename_res_ind( res_name, i, 0)) ; i++ ) {
		flags = parse_res_options( res_name, i, 1, blk_opts, "Unknown disk flag");

		if( flags & BF_IGNORE )
			continue;

		/* handle CD-roms */
		if( flags & BF_CD_ROM ) {
			flags &= ~BF_ENABLE_WRITE;
			if( (fd=disk_open( name, flags, &ro_fallback, 0 )) < 0 )
				continue;
			register_disk( "CD", name, "CD/DVD", fd, flags, 0 );
			continue;
		}

		switch( type ) {
		case kMacVolumes:
			setup_mac_disk( name, flags );
			break;

		case kLinuxDisks:
			if( (fd=disk_open( name, flags, &ro_fallback, 0 )) >= 0 ) {
				if( ro_fallback )
					flags &= ~BF_ENABLE_WRITE;
				register_disk( "Disk", name, NULL, fd, flags, get_file_size_in_blks(fd) );
			}
			break;
		}
	}

	/* handle boot override flag */
	for( bdev=s_all_disks; bdev && !(bdev->flags & BF_BOOT1); bdev=bdev->priv_next )
		;
	boot1 = bdev ? 1:0;
	for( bdev=s_all_disks; bdev && boot1; bdev=bdev->priv_next )
		if( !(bdev->flags & BF_BOOT1) )
			bdev->flags &= ~BF_BOOT;
}



/************************************************************************/
/*	checksum calculations						*/
/************************************************************************/

#if 0
/* Calculate a checksum for the HFS(+) MDB (master directory block). 
 * In particular, the modification date is included in the checksum.
 */
static int 
get_hfs_checksum( drive_t *drv, ulong *checksum )
{
	hfs_plus_mdb_t mdb_plus;
	hfs_mdb_t mdb;
	ulong val=0;

	blk_lseek( drv->fd, 2, SEEK_SET );
	read( drv->fd, &mdb, sizeof(mdb) );
	if( hfs_get_ushort(mdb.drSigWord) != HFS_SIGNATURE )
		return -1;

	/* printm("HFS volume detected\n"); */

	if( hfs_get_ushort(mdb.drEmbedSigWord) == HFS_PLUS_SIGNATURE ) {
		int sblock = hfs_get_ushort(mdb.drAlBlSt);
		sblock += (hfs_get_uint(mdb.drAlBlkSiz) / 512) * (hfs_get_uint(mdb.drEmbedExtent) >> 16);

		blk_lseek( drv->fd, sblock + 2, SEEK_SET );
		read( drv->fd, &mdb_plus, sizeof(mdb_plus) );

		if( mdb_plus.signature != HFS_PLUS_SIGNATURE ) {
			printm("HFS_PLUS_SIGNATURE expected\n");
			return -1;
		}
		val += calc_checksum( (char*)&mdb_plus, sizeof(mdb_plus) );

		/* printm("HFS+ volume detected\n"); */
	}
	val += calc_checksum( (char*)&mdb, sizeof(mdb_plus) );
	*checksum = val;

	/* printm("HFS-MDB checksum %08lX\n", *checksum ); */
	return 0;
}

static int
get_mdb_checksum( drive_t *drv, ulong *checksum )
{
	char buf[2048];

	if( !get_hfs_checksum( drv, checksum ) )
		return 0;

	if( !drv->locked || !(drv->flags & bf_force) ) {
		printm("Save session does not support r/w volumes which are not HFS(+)\n");
		return 1;
	}

	/* fallback - read the first four sectors */
	blk_lseek( drv->fd, 0, SEEK_SET );
	read( drv->fd, &buf, sizeof(buf) );

	*checksum = calc_checksum( buf, sizeof(buf) );
	return 0;
}
#endif


/************************************************************************/
/*	Global interface						*/
/************************************************************************/

bdev_desc_t *
bdev_get_next_volume( bdev_desc_t *last )
{
	bdev_desc_t *bdev = last ? last->priv_next : s_all_disks;
	for( ; bdev && bdev->priv_claimed; bdev=bdev->priv_next )
		;
	return bdev;
}

void
bdev_claim_volume( bdev_desc_t *bdev )
{
	bdev_desc_t *p;
	for( p=s_all_disks; p && p != bdev; p=p->priv_next )
		;
	p->priv_claimed = 1;
}

void
bdev_close_volume( bdev_desc_t *dev )
{
	bdev_desc_t **bd;

	for( bd=&s_all_disks; *bd; bd=&(**bd).priv_next ){
		if( *bd != dev )
			continue;

		/* unlink */
		*bd = (**bd).priv_next;

		free( dev->dev_name );
		if( dev->vol_name )
			free( dev->vol_name );
		
		if( dev->fd != -1 ) {
			/* XXX: the ioctl should only be done for block devices! */
			/* ioctl( dev->fd, BLKFLSBUF ); */
			close( dev->fd );
		}
		free( dev );
		return;
	}
	printm("bdev_close_volume: dev is not in the list!\n");
}

static int
blkdev_init( void )
{
	printm("\n");
	if( is_classic_boot() || is_osx_boot() ) {
		setup_disks("blkdev_mol", kMacVolumes );
		setup_disks("blkdev", kMacVolumes );
	}
	if( is_linux_boot() )
		setup_disks("blkdev", kLinuxDisks );
	printm("\n");
	return 1;
}

static void
blkdev_cleanup( void )
{
	bdev_desc_t *dev;

	for( dev=s_all_disks; dev; dev=dev->priv_next ) {
		if( dev->priv_claimed ){
			printm("Claimed disk not released properly!\n");
			continue;
		}

		/* Unclaimed disk */
		printm("Unclaimed disk '%s' released\n", dev->dev_name );
		bdev_close_volume( dev );
		blkdev_cleanup();
		break;
	}
}

driver_interface_t blkdev_setup = {
	"blkdev", blkdev_init, blkdev_cleanup
};
