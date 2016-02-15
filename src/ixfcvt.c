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

#include <unistd.h>

#include "ixfcvt.h"
#include "util.h"

#define REC_LEN_BYTES 6
#define DEF_BUFF_SIZE 4096
#define IXF_BAD_FORMAT "Not a valid IXF file"

static ssize_t get_record_len(const int fd);
static _Bool get_record(const int fd, unsigned char *buff, ssize_t rec_size);
static void append_column(struct column_desc *col, struct column_desc *head);
static void free_table(struct table_desc *tbl);
static void free_columns(struct column_desc *head);

/*
 * perform the conversion process
 *
 * ifd: input file of format IXF
 * ofd: output file for INSERT statements
 * cfd: output file for CREATE TABLE statement
 * table_name: user defined table name
 */
void parse_and_output(int ifd, int ofd, int cfd, const char *table_name)
{
	unsigned char *buff;
	ssize_t buff_size;
	ssize_t rec_len;
	struct table_desc *tbl;
	struct column_desc *col;

	buff = alloc_buff(DEF_BUFF_SIZE);
	buff_size = DEF_BUFF_SIZE;

	tbl = alloc_buff(sizeof(struct table_desc));
	tbl->c_head = NULL;

	while ((rec_len = get_record_len(ifd)) > 0) {
		if (rec_len > buff_size)
			buff = resize_buff(buff, rec_len);

		if (!get_record(ifd, buff, rec_len))
			err_exit(IXF_BAD_FORMAT);

		switch (*buff) {
		case 'H':
			break;
		case 'T':
			parse_t_record(buff, tbl, table_name);
			break;
		case 'C':
			col = alloc_buff(sizeof(struct column_desc));
			parse_c_record(buff, col);
			append_column(col, tbl->c_head);
			break;
		case 'D':
			/* produce INSERT statement according to D record */
			d_record_to_sql(ofd, buff, tbl, col_head);
			break;
		case 'A':
			break;
		default:
			err_exit(IXF_BAD_FORMAT);
		}
	}

	if (rec_len == -1)
		err_exit(IXF_BAD_FORMAT);

	/* output CREATE TABLE statement */
	t_desc_to_sql(cfd, tbl);

	free_table(tbl);
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

/* append a column description structure to the circular-linked list */
static void append_column(struct column_desc *col, struct column_desc *head)
{
	struct column_desc *node;

	if (head) {
		node = head;
		while (node->next != head)
			node = node->next;
		node->next = col;
	} else {
		head = col;
	}
	col->next = head;
}

/* free truct table_desc */
static void free_table(struct table_desc *tbl)
{
	free_buff(tbl->t_name);
	free_buff(tbl->t_pkname);
	free_columns(tbl->c_head);
	free_buff(tbl);
}

/* free list of column descriptions */
static void free_columns(struct column_desc *head)
{
	struct column_desc *node;

	while (head->next != head) {
		node = head->next;
		head->next = node->next;
		free_buff(node->c_name);
		free_buff(node);
	}
	free_buff(head->c_name);
	free_buff(head);
}
