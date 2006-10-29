/* Vector wrapper for regular read functions in other implementations */

#include "vec_wrap.h"

/* Read */
int wrap_read(bdev_desc_t *bdev, struct iovec *vec, int count)
{
	int i;
	u8 *buf, *buf_p;
	ssize_t total_bytes;	/* Total bytes requested */
//	ssize_t read_bytes;	/* Bytes requested from the read */

	printm("Starting vec_wrap read...");

	/* Find the toal bytes we need to read in */
	total_bytes = 0;
	for(i=0; i<count; i++)
	{
		if(total_bytes + vec[i].iov_len < SSIZE_MAX)
			total_bytes += vec[i].iov_len;
		else
			return -EINVAL;
	}

	/* Is this a valid assumption? */
	printm("Bytes to read: %i ", total_bytes);
	if(total_bytes % 512)
		printm("Warning: not a sector sized read\n");
	
	/* Allocate memory for the read */
	buf_p = buf = malloc(total_bytes);
	if(!buf)
		return -EINVAL;

	/* Do the read */
	if(bdev->real_read(bdev, buf, total_bytes) != total_bytes) {
		printm("Read failed ");
		return -EINVAL;
	}
	printm("Read complete ");

	/* Fill the vectors */
	for(i=0; i<count; i++) {
//		printm("Filling vector %i with %i bytes ", i, vec[i].iov_len);
		memcpy(vec[i].iov_base, buf_p, vec[i].iov_len);
		buf_p += vec[i].iov_len;
	}

	/* Free the memory */
	free(buf);
	printm("[ok]\n");
	return total_bytes;

}

/* Write */
int wrap_write(bdev_desc_t *bdev, struct iovec *vec, int count)
{
	int i;
	u8 *buf, *buf_p;
	ssize_t total_bytes;
	
	printm("Starting vec_wrap write...");

	/* Find the toal bytes we need to write out */
	total_bytes = 0;
	for(i=0; i<count; i++)
	{
		if(total_bytes + vec[i].iov_len < SSIZE_MAX)
			total_bytes += vec[i].iov_len;
		else
			return -EINVAL;
	}
	
	/* Is this a valid assumption? */
	printm("Bytes to write: %i ", total_bytes);
	if(total_bytes % 512)
		printm("Warning: not a sector sized write\n");
	
	/* Allocate memory for the write */
	buf_p = buf = malloc(total_bytes);
	
	/* Empty the vectors */
	for(i=0; i<count; i++) {
//		printm("Emptying vector %i with %i bytes ", i, vec[i].iov_len);
		memcpy(buf_p, vec[i].iov_base, vec[i].iov_len);
		buf_p += vec[i].iov_len;
	}

	/* Do the write */
	if(bdev->real_write(bdev, buf, total_bytes) != total_bytes) {
		printm("Write Failed ");
		return -EINVAL;
	}

	printm("Write complete ");
	
	/* Free the memory */
	free(buf);
	printm("[ok]\n");
	return total_bytes;

}
