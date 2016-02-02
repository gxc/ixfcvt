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
#include <strings.h>

#include "ixfcvt.h"
#include "util.h"

#define IXFTNAML_OFFSET 1
#define IXFTNAML_BYTES 3
#define IXFTNAME_OFFSET 4
#define IXFTCCNT_OFFSET 539
#define IXFTCCNT_BYTES 5
#define IXFTPKNM_OFFSET 576
#define TBL_ATTR_BUFF_SIZE 6

static void strip_ext(char *name, const char *ext);

/* parse a T record, store the info in a table struct */
void parse_table_record(const unsigned char *t_rec_buff, struct table *tbl,
			const char *table_name)
{
	static char buff[TBL_ATTR_BUFF_SIZE];
	const unsigned char *walker;
	int dat_name_len;
	int pk_name_len;

	if (!table_name) {
		memset(buff, 0x00, TBL_ATTR_BUFF_SIZE);
		memcpy(buff, t_rec_buff + IXFTNAML_OFFSET, IXFTNAML_BYTES);
		dat_name_len = str_to_long(buff);
		tbl->dat_name = alloc_buff(dat_name_len + 1);
		memcpy(tbl->dat_name, t_rec_buff + IXFTNAME_OFFSET,
		       dat_name_len);
		tbl->dat_name[dat_name_len] = '\0';
		strip_ext(tbl->dat_name, ".ixf");
	} else {
		tbl->dat_name = strdup(table_name);
		if (!tbl->dat_name)
			err_exit("virtual memory exhausted");
	}

	memcpy(buff, t_rec_buff + IXFTCCNT_OFFSET, IXFTCCNT_BYTES);
	tbl->col_num = str_to_long(buff);

	pk_name_len = 1;	/* 1 for '\0' */
	for (walker = t_rec_buff + IXFTPKNM_OFFSET; *walker; walker++)
		pk_name_len++;
	tbl->pk_name = alloc_buff(pk_name_len);
	memcpy(tbl->pk_name, t_rec_buff + IXFTPKNM_OFFSET, pk_name_len);
}

/* This function strips the trailing `.ixf' of `tbl->dat_name' */
static void strip_ext(char *name, const char *ext)
{
	char *ext_loc;
	int ext_len;
	int name_len;

	if (!name || !ext || (ext_len = strlen(ext)) == 0
	    || (name_len = strlen(name)) <= ext_len)
		return;

	ext_loc = name + name_len - ext_len;
	if (strcasecmp(ext_loc, ext) == 0)
		*ext_loc = '\0';
}
