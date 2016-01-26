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

#include <sys/types.h>

#define IXFDCOLS_OFFSET 8
#define VARCHAR_CUR_LEN_IND_BYTES 2
#define NULL_VAL_IND_BYTES 2

long parse_ixf_integer(const unsigned char *src, size_t bytes);
_Bool column_is_null(const unsigned char *null_ind);
char *decode_packed_decimal(const unsigned char *buff, size_t data_length);
size_t get_varchar_cur_len(const unsigned char *len_ind);
size_t varchar_len_ind_size(void);
