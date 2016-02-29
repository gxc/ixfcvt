/*
 * Copyright 2016 Guo, Xingchun <guoxingchun@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <unistd.h>

#include "ixfcvt.h"
#include "util.h"

#define SUMMARY_BYTES 7		/* record length(6) + record type) */

void get_ixf_summary(int fd, struct summary *sum)
{
	char buff[SUMMARY_BYTES];
	off_t orig;		/* original file offset */
	ssize_t max;		/* max record length */
	ssize_t len;		/* current record length */
	int c_cnt;		/* number of C records */
	long d_cnt;		/* number of D records */
	ssize_t n_read;		/* return value of read() */

	orig = seek_file(fd, 0, SEEK_CUR);	/* backup */

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
		max = len > max ? len : max;

		/* go to next record */
		seek_file(fd, len - 1, SEEK_CUR);
	}

	if (n_read == -1)
		err_exit("read");

	sum->s_ccnt = c_cnt;
	sum->s_dcnt = d_cnt;
	sum->s_recsz = max;

	seek_file(fd, orig, SEEK_SET);	/* restore */
}
