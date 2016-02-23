#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "ixfcvt.h"
#include "util.h"

#define SUMMARY_BYTES 7		/* record length(6) + record type) */

void get_summary(int fd, struct summary *sum)
{
	char buff[SUMMARY_BYTES];
	off_t orig;		/* original file offset */
	ssize_t max;		/* max record length */
	ssize_t len;		/* current record length */
	int c_cnt;		/* number of C records */
	long d_cnt;		/* number of D records */
	ssize_t n_read;		/* return value of read() */

	orig = seek_file(fd, 0, SEEK_CUR);

	max = 0;
	c_cnt = 0;
	d_cnt = 0L;
	seek_file(fd, 0, SEEK_SET);
	while ((n_read = read(fd, buff, SUMMARY_BYTES)) == SUMMARY_BYTES) {
		if (buff[SUMMARY_BYTES - 1] == 'C')
			++c_cnt;
		else if (buff[SUMMARY_BYTES - 1] == 'D')
			++d_cnt;

		buff[SUMMARY_BYTES - 1] = '\0';
		len = str_to_long(buff);
		if (len > max)
			max = len;
		seek_file(fd, len - 1, SEEK_CUR);
	}

	if (n_read == -1)
		err_exit("%s", strerror(errno));

	sum->s_c_cnt = c_cnt;
	sum->s_d_cnt = d_cnt;
	sum->s_rec_size = max;

	seek_file(fd, orig, SEEK_SET);
}
