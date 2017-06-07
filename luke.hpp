#include "dktool.hpp"
struct mmtp {
	char *magic;  // 6 bytes magic 
	unsigned int magic_read_size; /// magic has read size
	char type;	 // media type ： string , img , video
	bool is_first;  // is the first 
	bool has_read_first_and_type; // 
	unsigned int blank_read_size; //
	char *reserve; //
	unsigned int reserve_read_size;
	int32_t content_length; // contents whole size
	unsigned int content_length_has_read_size; // content length has read size;
	char *content;
};
void initilizer_mmtp(struct mmtp *mp);
void destory_mmtp(struct mmtp *mp) ;
/// clear and reset mp struct properties
/// read data acoordig the format.h file define
int mp_read(SOCKET sf_fd, int *filetype, struct mmtp *mp);
/// write data acoordig the format.h file define
int mp_write(SOCKET sf_fd, char *data, size_t n, int filetype);
