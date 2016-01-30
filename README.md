# ixfcvt

#### A tool for converting IBM PC/IXF format files to SQL statements

Building:

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
    ./ixfcvt -t NEW_TABLE -o insert_new_table.sql source.ixf
    ./ixfcvt -o insert_new_table.sql source.ixf
    ./ixfcvt source.ixf
    
Licensed under the Apache License, Version 2.0.

[ref: PC/IXF file format specification from IBM](https://www-01.ibm.com/support/knowledgecenter/api/content/SSEPGG_10.5.0/com.ibm.db2.luw.admin.dm.doc/doc/r0004668.html)
