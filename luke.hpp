#include <cstdint>
#include <cstddef>
#include <sys/_types/_ssize_t.h>
typedef int SOCKET;
struct mmtp {
	char *magic = NULL;  // 6 bytes magic 
	unsigned int magic_read_size = 0; /// magic has read size
	char type = 0;	 // media type ： string , img , video
	bool is_first = true;  // is the first
	bool has_read_first_and_type = false ; // 
	unsigned int blank_read_size = false; //
	char *reserve = NULL; //
	unsigned int reserve_read_size = 0;
	int32_t content_length = 0; // contents whole size
	unsigned int content_length_has_read_size = 0; // content length has read size;
	char *content = NULL ; 
	int32_t content_has_read_size = 0;
};
void initilizer_mmtp(struct mmtp *mp);
void destory_mmtp(struct mmtp **mp) ;
/// clear and reset mp struct properties
/// read data acoordig the format.h file define
int mp_read(SOCKET sf_fd, int *filetype, struct mmtp *mp);
/// write data acoordig the format.h file define
/// now use options as device token
/// we use 'reserve' section to store options.
/// so according to the format.hpp file 
/// the options' size must le than 4 bytes.
/// and as the decliare. we dont provide a param for user to apply the size of the options.
/// the options must be a c string that end with a '\0'
ssize_t mp_write(SOCKET sf_fd, const char *data, size_t n, int filetype, bool isfirst, const char *options);
/// copy file to the server
int mp_file_write(SOCKET sf_fd, const char * filename ,int filetype, const char *options);
