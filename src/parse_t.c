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
void parse_t_record(const unsigned char *rec, struct table_desc *tbl,
		    const char *table_name)
{
	char buff[TBL_ATTR_BUFF_SIZE];
	int t_name_len;
	int t_pkname_len;
	const unsigned char *walker;

	if (table_name) {
		tbl->t_name = strdup(table_name);
		if (!tbl->t_name)
			err_exit("strdup");
	} else {
		memset(buff, 0x00, TBL_ATTR_BUFF_SIZE);
		memcpy(buff, rec + IXFTNAML_OFFSET, IXFTNAML_BYTES);
		t_name_len = str_to_long(buff);
		tbl->t_name = alloc_buff(t_name_len + 1);
		memcpy(tbl->t_name, rec + IXFTNAME_OFFSET, t_name_len);
		tbl->t_name[t_name_len] = '\0';
		strip_ext(tbl->t_name, ".ixf");
	}

	memset(buff, 0x00, TBL_ATTR_BUFF_SIZE);
	memcpy(buff, rec + IXFTCCNT_OFFSET, IXFTCCNT_BYTES);
	tbl->t_ncols = str_to_long(buff);

	t_pkname_len = 0;
	for (walker = rec + IXFTPKNM_OFFSET; *walker; walker++)
		t_pkname_len++;
	tbl->t_pkname = alloc_buff(t_pkname_len + 1);
	memcpy(tbl->t_pkname, rec + IXFTPKNM_OFFSET, t_pkname_len);
	tbl->t_pkname[t_pkname_len] = '\0';
}

/* This function strips the trailing `.ixf' of `tbl->t_name' */
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
