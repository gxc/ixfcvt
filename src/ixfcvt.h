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

enum DB2_DATA_TYPE {
	/* not a complete list */
	CHAR = 452,
	VARCHAR = 448,
	SMALLINT = 500,
	INTEGER = 496,
	DECIMAL = 484,
	TIMESTAMP = 392
};

/* column descriptor */
/* TO-DO: IXFCDEFL, IXFCDEFV and IXFCKPOS */
struct column_desc {
	char *name;
	int type;
	size_t length;		/* when type is decimal, the first 3 digits is
				   the precision and last 2 digits for scale */
	long offset;		/* offset from the beginning of a D record */
	_Bool nullable;
	struct column_desc *next;
};

/* TO-DO: pk list */
struct table {
	char *dat_name;		/* original IXF file name, without ext `.ixf' */
	int col_num;		/* number of C records */
	char *pk_name;		/* name of the primary key */
};

void parse_table_record(const unsigned char *record, struct table *tbl);
void parse_column_desc_record(const unsigned char *c_rec_buff,
			      struct column_desc *col_desc);
void parse_data_record(const unsigned char *record,
		       const struct column_desc *col_desc_head);
char *define_table(char *buff, const struct table *tbl,
		   const struct column_desc *col_head);
char *format_insert(char *buff, const char *table_name,
		    const struct column_desc *col_head);
void data_record_to_sql(const unsigned char *d_rec_buff,
		        const struct column_desc *col_head);
