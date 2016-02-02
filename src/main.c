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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ixfcvt.h"
#include "util.h"

#ifdef DEBUG
#define VERSION "v0.20 <debug>"
#else
#define VERSION "v0.20"
#endif

static void ignore_lock_fail_or_exit(const char *filename);

int main(int argc, char *argv[])
{
	const char *const version_info = "\n\
ixfcvt Version %s\n\
A tool for converting IBM PC/IXF format files to SQL statements\n\
GitHub: https://github.com/gxc/ixfcvt\n\
\n\
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
\n\
";
	const char *const usage_info = "\n\
Usage: %s [-c CFILE] [-t TNAME] [-o OFILE] IXFFILE\n\
Convert an IBM PC/IXF format file to SQL statements\n\
\n\
    <IXFFILE>   input IXF format file, the data source\n\
Options:\n\
    -c CFILE    output CREATE TABLE statement to <CFILE> if specified\n\
    -h          display this help and exit\n\
    -o OFILE    output data of <IXFFILE> as INSERT statements to <OFILE>\n\
                If not specified, write to the standard output\n\
                <OFILE> should differ from <CFILE>\n\
    -t TNAME    use <TNAME> as the table name when output\n\
                If not specified, use data name of <IXFFILE>\n\
    -v          show version: \"ixfcvt %s by Guo, Xingchun\"\n\
\n\
";

	int errflg;		/* error on command line arguments */
	const char *ifile;	/* input IXF file as data source */
	const char *ofile;	/* output file to store INSERT statements */
	const char *cfile;	/* output file to store CREATE TABLE SQL */
	const char *tname;	/* user defined table name */
	int ifd;
	int ofd;
	int cfd;
	int oflags;
	mode_t mode;

	int c;

	setlocale(LC_ALL, "");
	if (argc == 1)
		usage(EXIT_FAILURE, usage_info, argv[0], VERSION);

	ifile = NULL;
	ofile = NULL;
	cfile = NULL;
	tname = NULL;
	errflg = 0;
	while ((c = getopt(argc, argv, ":c:o:t:hv")) != -1) {
		switch (c) {
		case 'c':
			cfile = optarg;
			break;
		case 'h':
			usage(errflg ? EXIT_FAILURE : EXIT_SUCCESS, usage_info,
			      argv[0], VERSION);
			break;
		case 'o':
			ofile = optarg;
			break;
		case 't':
			tname = optarg;
			break;
		case 'v':
			usage(EXIT_SUCCESS, version_info, VERSION);
			break;
		case ':':
			errflg++;
			err_msg("Option -%c requires an argument.", optopt);
			break;
		case '?':
			errflg++;
			err_msg("Unrecognized option: -%c.", optopt);
			break;
		}
	}

	if (optind == argc) {
		err_msg("No input IXF file specified.");
		errflg++;
	} else if (argc - optind > 1) {
		err_msg("Too many input files.");
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

	parse_and_output(ifd, ofd, cfd, tname);

	close_file(ifd);
	close_file(ofd);
	close_file(cfd);

	return 0;
}

/* prompt user to deside whether to ignore the file lock failure or to exit */
static void ignore_lock_fail_or_exit(const char *filename)
{
	_Bool is_ignored;

	fprintf(stdout, "Failed to lock file: %s\n"
		"Do you want to continue?", filename);
	is_ignored = prompt_y_or_n();

	if (!is_ignored)
		exit(EXIT_FAILURE);
}
