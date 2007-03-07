/* Create mol disk images 
 * Copyright 2006 - Joseph Jezak
 *
 * Based create routines on qemu's qemu-img
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include "byteorder.h"
#include <unistd.h>

int create_qcow(int, int64_t);
int create_raw(int, int64_t);
void help(void);

#define RAW_IMAGE 0
#define QCOW_IMAGE 1

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

/* QCOW defines from blk_qcow.h */
typedef struct QCowHeader {
    uint32_t magic;
    uint32_t version;
    uint64_t backing_file_offset;
    uint32_t backing_file_size;
    uint32_t mtime;
    uint64_t size; /* in bytes */
    uint8_t cluster_bits;
    uint8_t l2_bits;
    uint32_t crypt_method;
    uint64_t l1_table_offset;
} QCowHeader;

#define QCOW_MAGIC (('Q' << 24) | ('F' << 16) | ('I' << 8) | 0xfb)
#define QCOW_VERSION 1

#define QCOW_CRYPT_NONE 0
#define QCOW_CRYPT_AES  1

#define QCOW_OFLAG_COMPRESSED (1LL << 63)

/* Create empty qcow disk - size in bytes */
int create_qcow(int fd, int64_t size) {
	int header_size, l1_size, i, shift;
	QCowHeader header;
	uint64_t tmp;
    
	memset(&header, 0, sizeof(header));
	header.magic = cpu_to_be32(QCOW_MAGIC);
	header.version = cpu_to_be32(QCOW_VERSION);
	header.size = cpu_to_be64(size);
	header_size = sizeof(header);

	/* no backing file */
        header.cluster_bits = 12; /* 4 KB clusters */
        header.l2_bits = 9; /* 4 KB L2 tables */

	header_size = (header_size + 7) & ~7;
	shift = header.cluster_bits + header.l2_bits;
	l1_size = (size + (1LL << shift) - 1) >> shift;

	header.l1_table_offset = cpu_to_be64(header_size);
/* AES Stuff not yet implemented
	if (flags) 
		header.crypt_method = cpu_to_be32(QCOW_CRYPT_AES);
	else
*/
	header.crypt_method = cpu_to_be32(QCOW_CRYPT_NONE);

	/* write all the data */
	write(fd, &header, sizeof(header));
	lseek(fd, header_size, SEEK_SET);
	tmp = 0;
	for(i = 0;i < l1_size; i++) {
		write(fd, &tmp, sizeof(tmp));
	}
	return 0;
}

/* Create empty raw disk - size in bytes */
int create_raw(int fd, int64_t size) {
	ftruncate(fd, size);
	return 0;
}

void help(void) {
	printf ("Usage: mol-img [options] output.img\n");
	printf ("Options\n");
	printf ("\t--type=TYPE\tBuild a disk image of a certain type (listed below)\n");
	printf ("\t--size=SIZE\tSize in bytes, postfix with M or G for megabytes or gigabytes\n");
	printf ("\t--help\t\tThis help text\n\n");
	printf ("Available Image Types: raw, qcow\n");
	exit(0);
}

int main(int argc, char **argv) {
	int type = QCOW_IMAGE;
	char file[256] = "mol.img";
	/* Default to 512M */
	int64_t size = 512 * 1024 * 1024;
	int fd, len;
	int64_t multiplier = 1;

	/* Parse command line arguments */
	if (argc > 1) {
		int args;
		for (args = 1; args < argc; args++) {
			if (!strncmp(argv[args], "--type", 6)) {
				if (!strncmp(argv[args] + 7, "raw", 3))
				    	type = RAW_IMAGE;
				if (!strncmp(argv[args] + 7, "qcow", 3))
				    	type = QCOW_IMAGE;
			}
			else if (!strncmp(argv[args], "--size",6)) {
				len = strlen(argv[args] + 7);			
				if (!strncmp(argv[args] + 6 + len, "M", 1))
					multiplier = 1024 * 1024;		
				else if (!strncmp(argv[args] + 6 + len, "G", 1))
					multiplier = 1024 * 1024 * 1024;
				size = atoi(argv[args] + 7) * multiplier;
			}
			else if (!strncmp(argv[args], "--help",6)) {
				help();
			}
			/* Unknown option */
			else if (!strncmp(argv[args], "--",2)) {
				help();
			}
			/* Assume it's a filename */
			else {
				len = strlen(argv[args]);
				
				if (len < 256) {
					strncpy(file, argv[args], len);	
				}
				else {
					printf("Invalid filename!\n");
					exit(1);
				}
			}
		}
	}
	else {
		help();
	}
	
	/* Open the file */
	fd = open(file, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY | O_LARGEFILE, 0644);
	if (fd < 0)
		exit(-1);

	if (type == RAW_IMAGE)
		create_raw(fd, size);
	else if (type == QCOW_IMAGE)
		create_qcow(fd, size);
	else
		exit(-1);

	close(fd);

	exit(0);
}
