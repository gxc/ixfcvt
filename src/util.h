/*
 * util.h - header file for use with utility functions
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

#ifndef IXFCVT_UTIL_H_
#define IXFCVT_UTIL_H_

#include <stdbool.h>
#include <sys/types.h>

void err_msg(const char *format, ...);
void fmt_err_exit(const char *format, ...);
void err_exit(const char *msg);
void usage(int status, const char *format, ...);
long str_to_long(const char *str);
void *alloc_buff(size_t size);
void *resize_buff(void *buff, size_t new_size);
void free_buff(void *buff);
int open_file(const char *file, int oflags, mode_t mode);
void close_file(int fd);
off_t seek_file(int fd, off_t offset, int whence);
void write_file(int fd, const char *buff);
bool lock_entire_file(int fd, short lock_type);
bool prompt_y_or_n(void);
void show_progress(long cur, long sum);

#endif
