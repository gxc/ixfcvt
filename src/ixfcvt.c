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

#define REC_LEN_BYTES 6
#define DEF_BUFF_SIZE 4096

static ssize_t get_record_len(const int fd);
static _Bool get_record(const int fd, unsigned char *buff, ssize_t rec_size);
static struct column_desc *append_col_node(struct column_desc *rear);
static void free_col_desc(struct column_desc *head);
static void free_tbl(struct table *tbl);

/* Do the converting process */
void parse_and_output(int ifd, int ofd, int cfd, const char *table_name)
{
	unsigned char *buff;
	ssize_t buff_size;
	ssize_t rec_len;
	struct table *tbl;
	struct column_desc *col_head;
	struct column_desc *col_node;

	buff = malloc(DEF_BUFF_SIZE);
	if (!buff)
		err_exit("not enough memory available");
	buff_size = DEF_BUFF_SIZE;

	tbl = malloc(sizeof(struct table));
	if (!tbl)
		err_exit("not enough memory available");

	col_head = malloc(sizeof(struct column_desc));
	if (!col_head)
		err_exit("not enough memory available");
	/* only name of head can be NULL */
	col_head->name = NULL;
	col_node = col_head;

	while ((rec_len = get_record_len(ifd)) > 0) {
		if (rec_len > buff_size)
			buff = resize_buff(buff, rec_len);

		if (!get_record(ifd, buff, rec_len))
			err_exit("input file corrupted");

		switch (*buff) {
		case 'H':
			break;
		case 'T':
			parse_table_record(buff, tbl, table_name);
			break;
		case 'C':
			col_node = append_col_node(col_node);
			parse_column_desc_record(buff, col_node);
			break;
		case 'D':
			data_record_to_sql(ofd, buff, tbl, col_head);
			break;
		case 'A':
			break;
		default:
			err_exit("not a valid IXF file");
		}
	}

	if (rec_len == -1)
		err_exit("read record content failed, data wrong");

	table_desc_to_sql(cfd, tbl, col_head);

	free_col_desc(col_head);
	free_tbl(tbl);
}

/* Fills the buffer with a record, returns true on success, false on error. */
static _Bool get_record(const int fd, unsigned char *buff, ssize_t rec_size)
{
	if (read(fd, buff, rec_size) == rec_size)
		return 1;
	return 0;
}

/* Returns the size of next record on success, -1 on error, 0 on EOF. */
static ssize_t get_record_len(const int fd)
{
	static char buff[REC_LEN_BYTES + 1];
	ssize_t num_read;

	num_read = read(fd, buff, REC_LEN_BYTES);
	if (num_read == REC_LEN_BYTES)
		return str_to_long(buff);
	else
		return num_read;
}

/* Allocates a new column_desc struct and appends it to `rear' */
static struct column_desc *append_col_node(struct column_desc *rear)
{
	struct column_desc *node;

	node = malloc(sizeof(struct column_desc));
	if (!node)
		err_exit("not enough memory available");
	rear->next = node;
	node->next = NULL;

	return node;
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
