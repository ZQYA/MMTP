#include "luke.hpp"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
void initilizer_mmtp(struct mmtp *mp) {
	mp->magic = (char *)malloc(6);		
	bzero(mp->magic,6);
	mp->reserve = (char *)malloc(4);
	bzero(mp->reserve,4);
	mp->content = NULL; 
} 
void destory_mmtp(struct mmtp *mp) {
	if(mp->magic!=NULL) {
		free(mp->magic);
		mp->magic = NULL;
	}
	if(mp->reserve != NULL) {
		free(mp->reserve);
		mp->reserve = NULL;
	}
	if(mp->content != NULL) { 
		free(mp->content);
		mp->content = NULL;
	}
	if(mp!=NULL){
		free(mp);
		mp = NULL;
	}
}


int mp_read(SOCKET sf_fd, int *filetype, struct mmtp *mp) {
	while(mp->magic_read_size != 6) {
		int magic_need_read_size = 6-mp->magic_read_size;
		int read_size = dk_read(sf_fd,mp->magic+mp->magic_read_size,magic_need_read_size);	
		if(read_size<0) {
			bzero(mp->magic,6);	
			mp->magic_read_size = 0;
			close(sf_fd); /// ???? need change to shutdown 
		}else {
			if(0 == strncmp(mp->magic,"\r\nmmtp",6)) {
				mp->magic_read_size = 6;	
			}else {
				bzero(mp->magic,6);	
				mp->magic_read_size = 0;
			}
		}
	}
	
	if(!mp->has_read_first_and_type) {
		char tp_first_byte = 0;
		int readsize = dk_read(sf_fd,&tp_first_byte,1);	
		if(readsize<0) {
		}else {
			mp->type = tp_first_byte & 0x03; 		
			mp->has_read_first_and_type = true;
		}
	}

	if(mp->blank_read_size==0) {
		char tp_blank_byte = 0;
		int readsize = dk_read(sf_fd,&tp_blank_byte,1);	
		if(1==readsize) {
			mp->blank_read_size = 1;
		}
	}
	
	while(mp->reserve_read_size != 4) {
		int read_size = dk_read(sf_fd,(char*)mp->reserve+(mp->reserve_read_size),4-mp->reserve_read_size);
		if(read_size<0) {
		}else {
			mp->reserve_read_size += read_size;
		}
	}

	while(mp->content_length!= 4) {
		int read_size = dk_read(sf_fd,(char*)mp->content_length+(mp->content_length_has_read_size),4-mp->content_length_has_read_size);
		if(read_size<0) {
		}else {
			mp->content_length_has_read_size+= read_size;
		}
	}
	return 0;
}	
int mp_write(SOCKET sf_fd, char *data, size_t n, int filetype) {
	return 0;
}
