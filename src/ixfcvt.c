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
#define REC_LEN_BYTES_SIZE 6

static void free_col_desc(struct column_descriptor *head);
static void free_tbl(struct table *tbl);

/* return record length on success, return -1 on error, 0 on EOF */
/* output: data_buff and rec_type */
/* assume the size of data_buff is large enough */
ssize_t get_record(const int fd, unsigned char *data_buff, int *rec_type)
{
	ssize_t rec_len;

	if (read(fd, data_buff, REC_LEN_BYTES_SIZE) != REC_LEN_BYTES_SIZE)
		return 0;
	data_buff[REC_LEN_BYTES_SIZE] = '\0';
	rec_len = str_to_long((char *)data_buff);
	if (read(fd, data_buff, rec_len) != rec_len)
		return -1;

	*rec_type = *data_buff;
	return rec_len;
}

int main(void)
{
	const char *path = "../test/test.ixf";
	unsigned char buff[BUFF_SIZE];
	int fd;
	int open_flags = O_RDONLY;
	ssize_t rec_len;
	int rec_type;
	struct table *tbl;
	struct column_descriptor *col_head;
	struct column_descriptor *col_node;

	tbl = malloc(sizeof(struct table));
	if (!tbl)
		err_exit("not enough memory available");
	col_head = malloc(sizeof(struct column_descriptor));
	if (!col_head)
		err_exit("not enough memory available");

	if ((fd = open(path, open_flags)) == -1)
		err_exit("failed to open file: %s", path);

	col_node = col_head;
	memset(buff, 0x00, BUFF_SIZE);
	/* TO-DO: store H and A when pump use */
	while ((rec_len = get_record(fd, buff, &rec_type)) > 0) {
		switch (rec_type) {
		case 'H':
			break;
		case 'T':
			parse_table_record(buff, tbl);
			break;
		case 'C':
			col_node->next =
			    malloc(sizeof(struct column_descriptor));
			if (!col_node->next)
				err_exit("not enough memory available");
			col_node = col_node->next;
			col_node->next = NULL;
			parse_column_descriptor_record(buff, col_node);
			break;
		case 'D':
			parse_data_record(buff, col_head);
			break;
		case 'A':
			break;
		default:
			err_exit("wrong format detected");
		}
	}

	if (rec_len == -1)
		err_exit("read record content failed, data wrong");
	else			/* rec_len == 0 */
		printf("read all!\n");

	/* print the create table clauses */
	char buff_creat_tbl[2000];
	puts(define_table(buff_creat_tbl, tbl, col_head));

	free_tbl(tbl);
	free_col_desc(col_head);
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
static void free_col_desc(struct column_descriptor *head)
{
	struct column_descriptor *node;
	while (head->next) {
		node = head->next;
		head->next = node->next;
		free(node->name);
		free(node);
	}
	free(head);
}

