/*
 * util.c - definitions of utility functions
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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static bool is_blanks(const char *str);

/* write error message to stderr */
void err_msg(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	if (vfprintf(stderr, format, ap) < 0)
		err_exit("vfprintf");
	va_end(ap);
}

/* write error message to stderr and exit */
void fmt_err_exit(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	if (vfprintf(stderr, format, ap) > 0)
		putc('\n', stderr);
	va_end(ap);
	exit(EXIT_FAILURE);
}

/* print `msg:' and standard error message, then exit */
void err_exit(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

/* print USAGE message and exit with specified status */
void usage(int status, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	if (vfprintf(stderr, format, ap) > 0)
		putc('\n', stderr);
	va_end(ap);
	exit(status);
}

/*
 * This function is a wapper of the strtol function, returns
 * the long value that the base-10 numeric string represents.
 */
long str_to_long(const char *str)
{
	const char MSG[] = "Number parsing error";
	const int BASE = 10;
	long res;
	char *tailptr;

	if (!str || *str == '\0')
		fmt_err_exit("%s: null or empty string", MSG);
	if (is_blanks(str))
		return 0;

	errno = 0;
	res = strtol(str, &tailptr, BASE);
	if (errno == ERANGE)
		fmt_err_exit("%s (%s): %s", MSG, str, strerror(errno));
	if (*tailptr)
		fmt_err_exit("%s (%s): not a base-10 integer", MSG, str);

	return res;
}

/*
 * Return true if the string entirely consists of spaces, false otherwise.
 * assuming `str' not NULL
 */
static bool is_blanks(const char *str)
{
	while (*str)
		if (*str++ != ' ')
			return false;
	return true;
}

/* wrapper function for malloc; it exits if fail */
void *alloc_buff(size_t size)
{
	void *buff;

	buff = malloc(size);
	if (!buff)
		fmt_err_exit("virtual memory exhausted");

	return buff;
}

/* wrapper function for realloc; it exits if fail */
void *resize_buff(void *buff, size_t new_size)
{
	void *tmp;

	tmp = realloc(buff, new_size);
	if (!tmp)
		fmt_err_exit("virtual memory exhausted");

	return tmp;
}

/* free a buffer */
void free_buff(void *buff)
{
	if (buff)
		free(buff);
}

/* wrapper function for open; exit on error */
int open_file(const char *file, int oflags, mode_t mode)
{
	int fd;

	if ((fd = open(file, oflags, mode)) == -1)
		fmt_err_exit("%s: %s", file, strerror(errno));

	return fd;
}

/* wrappper function for close; exit on error */
void close_file(int fd)
{
	if (close(fd) == -1)
		err_exit("close");
}

/* wrapper function for lseek; exit on error */
off_t seek_file(int fd, off_t offset, int whence)
{
	off_t ret;

	ret = lseek(fd, offset, whence);
	if (ret == -1)
		err_exit("lseek");

	return ret;
}

/* write a null terminated buffer to a file */
void write_file(int fd, const char *buff)
{
	size_t to_write;
	ssize_t written;

	to_write = strlen(buff);
	if ((written = write(fd, buff, to_write)) == -1)
		fmt_err_exit("output file: %s", strerror(errno));
	else if (written != (ssize_t) to_write)
		fmt_err_exit("output file: resource limit reached");
}

/* locks an entire file, returns true on success, false otherwise */
bool lock_entire_file(int fd, short lock_type)
{
	struct flock lck;

	if (lock_type != F_RDLCK && lock_type != F_WRLCK
	    && lock_type != F_UNLCK)
		fmt_err_exit("wrong lock type: %hd", lock_type);

	lck.l_type = lock_type;
	lck.l_whence = SEEK_SET;
	lck.l_start = 0;
	lck.l_len = 0;

	if (fcntl(fd, F_SETLK, &lck) == 0)
		return true;
	else
		return false;
}

/* Prompts user to choose y or n, returns true or false respectively. */
bool prompt_y_or_n(void)
{
	int c;
	int ans;

	if (fputs(" (y/n): ", stderr) == EOF)
		err_exit("fputs");
	while (true) {
		c = fgetc(stdin);
		ans = tolower(c);

		/* eat the rest of the input, if any */
		while (c != EOF && c != '\n')
			c = fgetc(stdin);

		if (ans == 'y')
			return true;
		if (ans == 'n')
			return false;

		if (fputs("Please answer y or n: ", stderr) == EOF)
			err_exit("fputs");
	}
}

/* showcase the progress in percent (cur / sum) */
void show_progress(long cur, long sum)
{
	const char PROC_MSG[] = "Processing...";
	const char DONE_MSG[] = "Processing complete";
	static int pct;
	int tmp;

	tmp = (int)((double)cur / (double)sum * 100.0);
	if (tmp > pct) {
		pct = tmp;
		if (pct == 0)
			err_msg("%s\r", PROC_MSG);
		else if (pct == 100)
			err_msg("%s\n", DONE_MSG);
		else
			err_msg("%s (%d%%)\r", PROC_MSG, pct);
	}
}
