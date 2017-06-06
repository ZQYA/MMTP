#include "dktool.h"
/// read data acoordig the format.h file define
int mp_read(SOCKET sf_fd, char *buffer, size_t n, int *filetype);	
/// write data acoordig the format.h file define
int mp_write(SOCKET sf_fd, char *data, size_t n, int filetype);
