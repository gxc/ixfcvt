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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

static void print_errmsg(const char *format, va_list ap);
static _Bool is_blanks(const char *str);

/* write error message to stderr */
void err_msg(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	print_errmsg(format, ap);
	va_end(ap);
}

/* write error message to stderr and exit */
void err_exit(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	print_errmsg(format, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

/* print USAGE message and exit with specified status */
void usage(int status, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	print_errmsg(format, ap);
	va_end(ap);
	exit(status);
}

/* print the error message and a newline character */
static void print_errmsg(const char *format, va_list ap)
{
	vfprintf(stderr, format, ap);
	putc('\n', stderr);
}

/*
 * This function is a wapper of the strtol function, returns
 * the long value that the base-10 numeric string represents.
 */
long str_to_long(const char *str)
{
	long res;
	char *tailptr;
	const int base = 10;

	if (!str || *str == '\0')
		err_exit("%s: null or empty string", "str_to_long");
	if (is_blanks(str))
		return 0;

	errno = 0;
	res = strtol(str, &tailptr, base);
	if (errno)		/* ERANGE */
		err_exit("%s: overflow (%s)", "str_to_long", str);
	if (*tailptr)
		err_exit("%s: not a base-10 numeric string (%s)",
			 "str_to_long", str, res);

	return res;
}

/*
 * Return true if the string entirely consists of spaces, false otherwise.
 * assuming `str' not NULL
 */
static _Bool is_blanks(const char *str)
{
	while (*str)
		if (*str++ != ' ')
			return 0;
	return 1;
}

/* Returns a pointer to the newly allocated buffer */
void *resize_buff(void *buff, size_t new_size)
{
	void *tmp;

	tmp = realloc(buff, new_size);
	if (!tmp)
		err_exit("not enough memory available");

	return tmp;
}

/* open file, return file descripter; exit on error */
int open_file(const char *file, int oflags, mode_t mode)
{
	int fd;

	if ((fd = open(file, oflags, mode)) == -1)
		err_exit("%s: %s", file, strerror(errno));

	return fd;
}

/* close file; exit on error */
void close_file(int fd)
{
	if (close(fd) == -1)
		err_exit(strerror(errno));
}

/* write a null terminated buffer to a file */
void write_file(int fd, const char *buff)
{
	int to_write;
	int written;

	to_write = strlen(buff);
	if ((written = write(fd, buff, to_write)) == -1)
		err_exit("output file: %s", strerror(errno));
	else if (written != to_write)
		err_exit("output file: resource limit reached");
}
