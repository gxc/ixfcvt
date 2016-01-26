# ixfcvt

A tool for converting IBM PC/IXF format files to SQL statements


Compile:
    cd src && make release


Usage:
    ixfcvt [-c CFILE] [-t TNAME] [-o OFILE] IXFFILE

Argument:
    <IXFFILE>   input IXF format file, the data source
Options:
    -c CFILE    output CREATE TABLE statement to <CFILE> if specified
    -h          display this help and exit
    -o OFILE    output data of <IXFFILE> as INSERT statements to <OFILE>
                If not specified, write to the standard output
                <OFILE> should differ from <CFILE>
    -t TNAME    use <TNAME> as the table name when output
                If not specified, use data name of <IXFFILE>
    -v          show version: "ixfcvt V0.10 by Guo, Xingchun"

Example:
    ./ixfcvt -c create_table.sql -t NEW_TABLE -o insert_new_table.sql source.ixf

Licensed under the Apache License, Version 2.0.

