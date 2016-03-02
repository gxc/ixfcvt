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
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ixfcvt.h"
#include "util.h"

#ifdef DEBUG
#define VERSION "v0.63 <debug>"
#else
#define VERSION "v0.63"
#endif

#define MAX_COMMIT_SIZE 0xFFFF

static void ignore_lock_fail_or_exit(const char *filename);

int main(int argc, char *argv[])
{
	char *const version_info = "\n\
ixfcvt Version %s\n\
A tool for converting IBM PC/IXF format files to SQL statements\n\
\n\
GitHub: https://github.com/gxc/ixfcvt\n\
\n\
Copyright 2016 Guo, Xingchun <guoxingchun@gmail.com>\n\
\n\
Licensed under the Apache License, Version 2.0 (the \"License\");\n\
you may not use this file except in compliance with the License.\n\
You may obtain a copy of the License at\n\
\n\
  http://www.apache.org/licenses/LICENSE-2.0\n\
\n\
Unless required by applicable law or agreed to in writing, software\n\
distributed under the License is distributed on an \"AS IS\" BASIS,\n\
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n\
See the License for the specific language governing permissions and\n\
limitations under the License.\n\
";
	char *const usage_info = "\n\
Usage: %s [-c CFILE] [-t TNAME] [-e] [-o OFILE] [-s SIZE] [IXFFILE]\n\
Convert an IBM PC/IXF format file to SQL statements\n\
\n\
Argument:\n\
    <IXFFILE>     input IXF format file, the data source\n\
Options:\n\
    -c <CFILE>    output CREATE TABLE statement to <CFILE> if specified\n\
    -e            escape backslash(\\), or use it as literal by default\n\
    -h            display this help and exit\n\
    -o <OFILE>    output data of <IXFFILE> as INSERT statements to <OFILE>\n\
                  If not specified, write to the standard output\n\
                  <IXFFILE>, <OFILE> and <CFILE> must differ from each other\n\
    -s <SIZE>     issue a COMMIT every <SIZE> rows (default 1000)\n\
                  If <SIZE> is 0, no COMMIT statement will be issued\n\
    -t <TNAME>    use <TNAME> as the table name when output\n\
                  If not specified, use data name of <IXFFILE>\n\
    -v            show version: \"ixfcvt %s by Guo, Xingchun\"\n\
";

	const char *ifile;	/* input IXF file as data source */
	const char *ofile;	/* output file to store INSERT statements */
	const char *cfile;	/* output file to store CREATE TABLE SQL */
	char *tname;		/* user defined table name */
	long commit_size;	/* commit size */
	_Bool esc_bs;		/* whether escape backslash */

	struct summary sum;
	int errflg;		/* error on command line arguments */
	int ifd;
	int ofd;
	int cfd;
	int oflags;
	mode_t mode;

	int c;

	setlocale(LC_ALL, "");
	if (argc == 1)
		usage(EXIT_FAILURE, usage_info, argv[0], VERSION);

	errflg = 0;
	ifile = NULL;
	ofile = NULL;
	cfile = NULL;
	tname = NULL;
	esc_bs = 0;
	commit_size = 1000L;
	while ((c = getopt(argc, argv, ":c:o:s:t:ehv")) != -1) {
		switch (c) {
		case 'c':
			cfile = optarg;
			break;
		case 'e':
			esc_bs = 1;
			break;
		case 'h':
			usage(errflg ? EXIT_FAILURE : EXIT_SUCCESS, usage_info,
			      argv[0], VERSION);
			break;
		case 'o':
			ofile = optarg;
			break;
		case 's':
			commit_size = str_to_long(optarg);
			if (commit_size < 0 || commit_size > MAX_COMMIT_SIZE)
				fmt_err_exit
				    ("%s: Commit size must be between 0 and %hu",
				     argv[0], MAX_COMMIT_SIZE);
			break;
		case 't':
			tname = optarg;
			break;
		case 'v':
			usage(EXIT_SUCCESS, version_info, VERSION);
			break;
		case ':':
			errflg++;
			err_msg("Option -%c requires an argument\n", optopt);
			break;
		case '?':
			errflg++;
			err_msg("Unrecognized option: -%c\n", optopt);
			break;
		}
	}

	if (optind == argc) {
		err_msg("%s", "No input IXF file specified");
		errflg++;
	} else if (argc - optind > 1) {
		err_msg("%s", "Too many input files specified");
		errflg++;
	} else {
		ifile = argv[optind];
	}

	if (errflg)
		usage(EXIT_FAILURE, usage_info, argv[0], VERSION);

	oflags = O_WRONLY | O_CREAT | O_TRUNC;
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	ifd = open_file(ifile, O_RDONLY, 0);
	if (!lock_entire_file(ifd, F_RDLCK))
		ignore_lock_fail_or_exit(ifile);

	if (ofile) {
		ofd = open_file(ofile, oflags, mode);
		if (!lock_entire_file(ofd, F_WRLCK))
			ignore_lock_fail_or_exit(ofile);
	} else {
		ofd = STDOUT_FILENO;
	}

	if (cfile) {
		cfd = open_file(cfile, oflags, mode);
		if (!lock_entire_file(cfd, F_WRLCK))
			ignore_lock_fail_or_exit(cfile);
	} else {
		cfd = open_file("/dev/null", O_WRONLY, 0);
	}

	sum.s_cmtsz = (int)commit_size;
	sum.s_tname = tname;
	sum.s_escbs = esc_bs;

	err_msg("%s\r", "Preparing...");
	get_ixf_summary(ifd, &sum);
	parse_and_output(ifd, ofd, cfd, &sum);

	close_file(ifd);
	close_file(ofd);
	close_file(cfd);

	return 0;
}

/* prompt user to deside whether to ignore the file lock failure or to exit */
static void ignore_lock_fail_or_exit(const char *filename)
{
	_Bool is_ignored;

	err_msg("Failed to lock file: %s\nDo you want to continue?", filename);
	is_ignored = prompt_y_or_n();

	if (!is_ignored)
		exit(EXIT_FAILURE);
}
