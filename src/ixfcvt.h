/*
 * ixfcvt.h - definitions of structures and declarations of skeleton functions
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

#ifndef IXFCVT_IXFCVT_H_
#define IXFCVT_IXFCVT_H_

#include <sys/types.h>

#define E_DATA_TYPE_NOT_IMPL "data type (%d) not yet implemented"

enum DB2_DATA_TYPE {
	/* *not* a complete list */
	CHAR = 452,
	VARCHAR = 448,
	SMALLINT = 500,
	INTEGER = 496,
	BIGINT = 492,
	DECIMAL = 484,
	DATE = 384,
	TIME = 388,
	TIMESTAMP = 392,
	FLOATING_POINT = 480	/* DOUBLE or REAL */
};

/* requirements and basic info of input IXF file */
struct summary {
	int s_cmtsz;		/* commit size */
	char *s_tname;		/* user-defined table name */
	_Bool s_escbs;		/* escape backslash */
	int s_ccnt;		/* C record count */
	long s_dcnt;		/* D record conut */
	size_t s_recsz;		/* maximum record size */
};

/* TO-DO: IXFCDEFL, IXFCDEFV */
/* singly linked list of column definition */
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

void get_ixf_summary(int fd, struct summary *sum);
void parse_and_output(int ifd, int ofd, int cfd, const struct summary *sum);
void parse_t_record(const unsigned char *rec, struct table_desc *tbl,
		    const char *table_name);
void parse_c_record(const unsigned char *rec, struct column_desc *col);
void parse_d_record(const unsigned char *record,
		    const struct column_desc *col_head);
void table_desc_to_sql(int fd, const struct table_desc *tbl);
void d_record_to_sql(int ofd, const unsigned char *rec,
		     const struct summary *sum, const struct table_desc *tbl);

#endif
