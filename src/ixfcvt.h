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

#ifndef IXFCVT_IXFCVT_H_
#define IXFCVT_IXFCVT_H_

#include <sys/types.h>

enum DB2_DATA_TYPE {
	/* not a complete list */
	CHAR = 452,
	VARCHAR = 448,
	SMALLINT = 500,
	INTEGER = 496,
	DECIMAL = 484,
	TIMESTAMP = 392
};

/* TO-DO: IXFCDEFL, IXFCDEFV */
/* circular-linked list of column definition */
struct column_desc {
	char *c_name;		/* column name */
	int c_type;		/* data type */
	size_t c_len;		/* data length */
	/* If data type is decimal, the first 3 digits of `c_len'
	   is the precision, the last 2 digits is the scale */
	off_t c_offset;		/* offset from beginning of a D record */
	_Bool c_nullable;	/* whether accepts null values */
	int c_pkpos;		/* position in primary key */
	struct column_desc *next;
};

struct table_desc {
	char *t_name;		/* table name, data name as default */
	char *t_pkname;		/* primary key name */
	int t_ncols;		/* number of columns */
	struct column_desc *c_head;	/* point to first column_desc */
};

void parse_and_output(int ifd, int ofd, int cfd, const char *table_name);
void parse_t_record(const unsigned char *rec, struct table_desc *tbl, const char *table_name);
void parse_c_record(const unsigned char *rec, struct column_desc *col_desc);
void parse_d_record(const unsigned char *record, const struct column_desc *col_desc_head);
void t_record_to_sql(int fd, const struct table *tbl, const struct column_desc *col_head);
void d_record_to_sql(int fd, const unsigned char *d_rec_buff, const struct table *tbl, const struct column_desc *col_head);

#endif
