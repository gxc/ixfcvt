/*
 * ixfcvt.c - extract and distribute records
 *
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

static ssize_t get_record_len(int fd);
static void get_record(int fd, unsigned char *rec, size_t rec_size);
static void append_column(struct column_desc *col, struct table_desc *tbl);
static void free_table(struct table_desc *tbl);
static void free_columns(struct column_desc *head);

/*
 * perform the conversion process
 *
 * ifd: input file of format IXF
 * ofd: output file for INSERT statements
 * cfd: output file for CREATE TABLE statement
 */
void parse_and_output(int ifd, int ofd, int cfd, const struct summary *sum)
{
	struct table_desc *tbl;
	struct column_desc *col;
	unsigned char *rec;
	ssize_t rec_len;

	rec = alloc_buff(sum->s_recsz);
	tbl = alloc_buff(sizeof(struct table_desc));
	tbl->c_head = NULL;

	while ((rec_len = get_record_len(ifd)) > 0) {
		get_record(ifd, rec, (size_t) rec_len);

		switch (*rec) {
		case 'H':
			break;
		case 'T':
			parse_t_record(rec, tbl, sum->s_tname);
			break;
		case 'C':
			col = alloc_buff(sizeof(struct column_desc));
			parse_c_record(rec, col);
			append_column(col, tbl);
			break;
		case 'D':
			d_record_to_sql(ofd, rec, sum, tbl);
			break;
		case 'A':
			break;
		default:
			fmt_err_exit("Unknown record type encountered: %c",
				     *rec);
		}
	}

	if (rec_len == -1)
		err_exit("read");

	/* output CREATE TABLE statement */
	table_desc_to_sql(cfd, tbl);

	free_buff(rec);
	free_table(tbl);
}

/* Fills the buffer with a record, exits on error. */
static void get_record(int fd, unsigned char *rec, size_t rec_size)
{
	ssize_t n_read;

	n_read = read(fd, rec, rec_size);
	if (n_read == -1)
		err_exit("read");
	else if ((size_t) n_read < rec_size)
		fmt_err_exit("%s", "Error reading input file");

}

/* Returns the size of next record on success, -1 on error, 0 on EOF. */
static ssize_t get_record_len(int fd)
{
	static char buff[REC_LEN_BYTES + 1];
	ssize_t num_read;

	num_read = read(fd, buff, REC_LEN_BYTES);
	if (num_read == REC_LEN_BYTES)
		return str_to_long(buff);
	else
		return num_read;
}

/* append a column description structure to the singly-linked list */
static void append_column(struct column_desc *col, struct table_desc *tbl)
{
	struct column_desc *node;

	if (tbl->c_head) {
		node = tbl->c_head;
		while (node->next)
			node = node->next;
		node->next = col;
	} else {
		tbl->c_head = col;
	}
	col->next = NULL;
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

	while (head->next) {
		node = head->next;
		head->next = node->next;
		free_buff(node->c_name);
		free_buff(node);
	}
	free_buff(head->c_name);
	free_buff(head);
}
