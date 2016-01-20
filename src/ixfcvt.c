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

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "ixfcvt.h"

#ifndef BUFF_SIZE
#define BUFF_SIZE 10240
#endif
#define REC_LEN_BYTES 6

#include <errno.h>
static ssize_t get_record(const int fd, unsigned char *data_buff);
static void free_col_desc(struct column_desc *head);
static void free_tbl(struct table *tbl);

/*
 * Fills the buffer with a record, returns the length of the
 * record on success, return -1 on error, 0 on EOF.
 * Assumes that the buffer is large enough.
 */
static ssize_t get_record(const int fd, unsigned char *data_buff)
{
	static char recl_buff[REC_LEN_BYTES + 1];
	ssize_t rec_len;

	if (read(fd, recl_buff, REC_LEN_BYTES) != REC_LEN_BYTES)
		return 0;
	rec_len = str_to_long(recl_buff);
	if (read(fd, data_buff, rec_len) != rec_len)
		return -1;

	return rec_len;
}

/* TO-DO: provide an interface */
int main(int argc, char *argv[])
{
	char path[1000];
	unsigned char buff[BUFF_SIZE];
	int fd;
	int open_flags = O_RDONLY;
	ssize_t rec_len;
	int rec_type;
	struct table *tbl;
	struct column_desc *col_head;
	struct column_desc *col_node;

	/* for testing */
	if (argc > 1 && access(argv[1], R_OK) == 0)
		strcpy(path, argv[1]);
	else
		strcpy(path, "../test/test.ixf");

	tbl = malloc(sizeof(struct table));
	if (!tbl)
		err_exit("not enough memory available");
	/* TO-DO: change to a lighter head? */
	col_head = malloc(sizeof(struct column_desc));
	if (!col_head)
		err_exit("not enough memory available");
	else
		/* only name of head is NULL */
		col_head->name = NULL;

	if ((fd = open(path, open_flags)) == -1) {
		err_exit("failed to open file: %s", path);
	}

	col_node = col_head;
	memset(buff, 0x00, BUFF_SIZE);
	/* TO-DO: refactory to a function, store H and A when pump use */
	while ((rec_len = get_record(fd, buff)) > 0) {
		rec_type = *buff;
		switch (rec_type) {
		case 'H':
			break;
		case 'T':
			parse_table_record(buff, tbl);
			break;
		case 'C':
			col_node->next = malloc(sizeof(struct column_desc));
			if (!col_node->next)
				err_exit("not enough memory available");
			col_node = col_node->next;
			col_node->next = NULL;
			parse_column_desc_record(buff, col_node);
			break;
		case 'D':
			data_record_to_sql(buff, tbl->dat_name, col_head);
			break;
		case 'A':
			break;
		default:
			err_exit("wrong format detected");
		}
	}

	if (rec_len == -1)
		err_exit("read record content failed, data wrong");

	/* TO-DO: modify this, no buff_def */
	/* print the create table clauses */
	char buff_def[5000];
	puts(define_table(buff_def, tbl, col_head));

	free_col_desc(col_head);
	free_tbl(tbl);
	if (close(fd) == -1)
		err_exit("failed to close file: %s", path);

	return 0;
}

/* free the T record */
static void free_tbl(struct table *tbl)
{
	free(tbl->dat_name);
	free(tbl->pk_name);
	free(tbl);
}

/* free C records */
static void free_col_desc(struct column_desc *head)
{
	struct column_desc *node;
	while (head->next) {
		node = head->next;
		head->next = node->next;
		free(node->name);
		free(node);
	}
	free(head);
}
