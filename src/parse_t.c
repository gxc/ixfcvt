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

#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "ixfcvt.h"

#define IXFTNAML_OFFSET 1
#define IXFTNAML_BYTES 3
#define IXFTNAME_OFFSET 4
#define IXFTCCNT_OFFSET 539
#define IXFTCCNT_BYTES 5
#define IXFTPKNM_OFFSET 576
#define TBL_ATTR_BUFF_SIZE 6

/* parse a T record, store the info in a table struct */
void parse_table_record(const unsigned char *t_rec_buff, struct table *tbl)
{
	static unsigned char buff[TBL_ATTR_BUFF_SIZE];
	const unsigned char *walker;
	int tbl_name_len;
	int pk_name_len;

	memset(buff, 0x00, TBL_ATTR_BUFF_SIZE);
	memcpy(buff, t_rec_buff + IXFTNAML_OFFSET, IXFTNAML_BYTES);
	tbl_name_len = str_to_long((char *)buff);
	tbl->tbl_name = malloc(tbl_name_len + 1);
	if (!tbl->tbl_name)
		err_exit("not enough memory available");
	memcpy(tbl->tbl_name, t_rec_buff + IXFTNAME_OFFSET, tbl_name_len);
	tbl->tbl_name[tbl_name_len] = '\0';

	memcpy(buff, t_rec_buff + IXFTCCNT_OFFSET, IXFTCCNT_BYTES);
	tbl->c_rec_cnt = str_to_long((char *)buff);

	pk_name_len = 1;	/* 1 for '\0' */
	for (walker = t_rec_buff + IXFTPKNM_OFFSET; *walker; walker++)
		pk_name_len++;
	tbl->pk_name = malloc(pk_name_len);
	if (!tbl->pk_name)
		err_exit("not enough memory available");
	memcpy(tbl->pk_name, t_rec_buff + IXFTPKNM_OFFSET, pk_name_len);
}