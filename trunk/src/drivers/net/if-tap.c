/* 
 *   Creation Date: <2004/03/28 00:16:00 samuel>
 *   Time-stamp: <2004/06/12 15:11:06 samuel>
 *   
 *	<if-tap.c>
 *	
 *	ethertap packet driver (obsolete)
 *   
 *   Copyright (C) 1999-2004 Samuel Rydh (samuel@ibrium.se)
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *   
 */

#include "mol_config.h"
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include "enet.h"
#include "res_manager.h"
#include <linux/version.h>

#ifndef NETLINK_GENERIC
#define NETLINK_GENERIC 16
#endif

static const char		def_hw_addr[6] = { 0, 0, 0xDE, 0xAD, 0xBE, 0xEF };

static void
tap_preconfigure( enet_iface_t *is )
{
	memcpy( is->c_macaddr, def_hw_addr, 6 );
	is->c_macaddr[5] += g_session_id;
	is->client_ip += g_session_id;
	/* XXX fixme - default IP assignment is not good */
}

static int
tap_open( enet_iface_t *is )
{
	struct sockaddr_nl nladdr;
	int fd, tapnum=0;
	unsigned char *p = is->c_macaddr;
	
	if( is->iface_name[0] ) {
		if( sscanf(is->iface_name, "tap%d", &tapnum) == 1 ) {
			if( tapnum<0 || tapnum>15 ) {
				printf("Invalid tap device %s. Using tap0 instead\n", is->iface_name );
				is->iface_name[0] = 0;
			}
		} else {
			printm("Bad tapdevice interface '%s'\n", is->iface_name );
			printm("Using default tap device (tap0)\n");
			is->iface_name[0] = 0;
		}
	}
	if( !is->iface_name[0] ) {
		tapnum = 0;
		sprintf( is->iface_name, "tap0" );
	}
	
	/* verify that the device is up and running */
	if( check_netdev(is->iface_name) )
		return 1;

	if( (fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_GENERIC+tapnum)) < 0 ) {
		perrorm("socket");
		printm("Does the kernel lack netlink support (CONFIG_NETLINK)?\n");
		return 1;
	}
	memset( &nladdr, 0, sizeof(nladdr) );
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_groups = ~0;
	nladdr.nl_pid = ((u32)p[2]<<24) | ((u32)p[3]<<16) | ((u32)p[4]<<8) | p[5];
	if( bind(fd, (struct sockaddr*)&nladdr, sizeof(nladdr)) < 0 ) {
		perrorm("bind");
		close(fd);
		return 1;
	}
	is->packet_pad = TAP_PACKET_PAD;

	netif_open_common( is, fd );
	return 0;
}

static void
tap_close( enet_iface_t *is )
{
	netif_close_common( is );
}


static packet_driver_t tap_pd = {
	.name		= "tap",
	.flagstr	= "-tap",
	.id 		= TAP_PACKET_DRIVER_ID,
	.preconfigure	= tap_preconfigure,
	.open 		= tap_open,
	.close		= tap_close,
};

DECLARE_PACKET_DRIVER( init_tap, tap_pd );
