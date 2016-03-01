# ixfcvt

#### A tool for converting IBM PC/IXF format files to SQL statements

Building:

    on Linux: cd src && make
    on AIX:   cd src && cp Makefile.AIX Makefile && make
Usage:

    ixfcvt [-c CFILE] [-t TNAME] [-o OFILE] [-s SIZE] [IXFFILE]
Argument:

    <IXFFILE>   input IXF format file, the data source
Options:

    -c CFILE    output CREATE TABLE statement to <CFILE> if specified
    -h          display this help and exit
    -o OFILE    output data of <IXFFILE> as INSERT statements to <OFILE>
                If not specified, write to the standard output
                <IXFFILE>, <OFILE> and <CFILE> must differ from each other
    -s SIZE     issue a COMMIT every <SIZE> rows (default 1000)
                If <SIZE> is 0, no COMMIT statement will be issued
    -t TNAME    use <TNAME> as the table name when output
                If not specified, use data name of <IXFFILE>
    -v          show version: "ixfcvt V0.10 by Guo, Xingchun"
Examples:

    ./ixfcvt -c create_table.sql -t tableA -o tableA.data.sql -s 2000 source.ixf
    ./ixfcvt -c create_table.sql -o insert_new_table.sql source.ixf
    ./ixfcvt -t NEW_TABLE -o insert_new_table.sql source.ixf
    ./ixfcvt -o insert_new_table.sql source.ixf
    ./ixfcvt source.ixf

Licensed under the Apache License, Version 2.0.

[ref: PC/IXF file format specification from IBM](https://www-01.ibm.com/support/knowledgecenter/api/content/SSEPGG_10.5.0/com.ibm.db2.luw.admin.dm.doc/doc/r0004668.html)
