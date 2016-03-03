# ixfcvt

### A tool for converting an IBM PC/IXF format file to SQL statements

##### Currently supports the following DB2 data types:
- CHAR
- VARCHAR
- SMALLINT
- INTEGER
- BIGINT
- DECIMAL
- DATE
- TIME
- TIMESTAMP

##### Building:
    on Linux:   cd src && make
    on AIX:     cd src && cp Makefile.AIX Makefile && make

##### Usage:
    ixfcvt [-c CFILE] [-t TNAME] [-e] [-o OFILE] [-s SIZE] [IXFFILE]
###### Argument:
    <IXFFILE>   input IXF format file, the data source
###### Options:
    -c CFILE    output CREATE TABLE statement to <CFILE> if specified
    -e          escape backslash(\\), or use it as literal by default
    -h          display this help and exit
    -o OFILE    output data of <IXFFILE> as INSERT statements to <OFILE>
                If not specified, write to the standard output
                <IXFFILE>, <OFILE> and <CFILE> must differ from each other
    -s SIZE     issue a COMMIT every <SIZE> rows (default 1000)
                If <SIZE> is 0, no COMMIT statement will be issued
    -t TNAME    use <TNAME> as the table name when output
                If not specified, use data name of <IXFFILE>
    -v          show version: "ixfcvt V0.70 by Guo, Xingchun"

##### Examples:
    ./ixfcvt -c create_table.sql -t tableA -e -o tableA.data.sql -s 2000 source.ixf
    ./ixfcvt -c create_table.sql -o insert_table.sql source.ixf
    ./ixfcvt -t tableB -o insert_table.sql source.ixf
    ./ixfcvt -e -o insert_table.sql source.ixf
    ./ixfcvt -o insert_table.sql source.ixf
    ./ixfcvt source.ixf

##### References:
- [PC/IXF file format specification from IBM](https://www-01.ibm.com/support/knowledgecenter/api/content/SSEPGG_10.5.0/com.ibm.db2.luw.admin.dm.doc/doc/r0004668.html)
- [PC/IXF data type descriptions from
IBM](https://www-01.ibm.com/support/knowledgecenter/api/content/SSEPGG_10.5.0/com.ibm.db2.luw.admin.dm.doc/doc/r0008742.html)

#####  License:
[Licensed under the Apache License, Version 2.0.](http://www.apache.org/licenses/LICENSE-2.0)

