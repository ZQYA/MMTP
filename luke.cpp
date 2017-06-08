#include "luke.hpp"
#include "dktool.hpp"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <strings.h>
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
			mp->is_first = tp_first_byte & 0x04;
			mp->type = tp_first_byte & 0x03; 		
			mp->has_read_first_and_type = true;
		}
	}

	if(mp->blank_read_size==0) {
		char tp_blank_byte = 0;
		int read_size = dk_read(sf_fd,&tp_blank_byte,1);	
		if (read_size<0) {
		}else  {
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

	while(mp->content_length_has_read_size!= 4) {
		int read_size = dk_read(sf_fd,(char*)mp->content_length+(mp->content_length_has_read_size),4-mp->content_length_has_read_size);
		if(read_size<0) {
		}else {
			mp->content_length_has_read_size+= read_size;
		}
	}
	
	while(mp->all_content_length_has_read_size!= 4) {
		int read_size = dk_read(sf_fd,(char*)mp->all_content_length+(mp->all_content_length_has_read_size),4-mp->all_content_length_has_read_size);
		if(read_size<0) {
		}else {
			mp->all_content_length_has_read_size+= read_size;
		}
	}
	

	if(mp->is_first&&mp->content!=NULL){
		free(mp->content);
		mp->content = NULL;
		mp->content = (char*)malloc(mp->all_content_length);
   	}	
	
	while(mp->content_has_read_size!=mp->content_length){
		int read_size =	dk_read(sf_fd,mp->content+mp->content_has_read_size,mp->content_length-mp->content_has_read_size);
		if( read_size < 0  ) {
		}else {
			mp->content_has_read_size+=read_size;
		}
	}
	return 0;
}	
int mp_write(SOCKET sf_fd, char *data, size_t n, int filetype, bool isfirst) {
	char *content = (char *)malloc(n + 16);	
	bzero(content,n+16);
	strcat(content,"\r\nmmtp");
	char tp_first_byte = 0x00;
	tp_first_byte |= isfirst?0x04:0x00;
	tp_first_byte |= filetype;
	memcpy(content+6,&tp_first_byte,1);
	int32_t content_length = n;
	memcpy(content+12,&content_length,4);
	memcpy(content+16,data,n);
	return 	dk_write(sf_fd, content, n+16);
}
