
long parse_ixf_integer(const unsigned char *src, size_t bytes);
_Bool column_is_null(const unsigned char *null_ind);
char *decode_packed_decimal(const unsigned char *buff, size_t data_length);
