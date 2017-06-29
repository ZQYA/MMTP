#include "luke.hpp"
#include "dktool.hpp"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <cstdio>
const size_t file_write_segment_max = 1024;
void initilizer_mmtp(struct mmtp *mp) {
	mp->magic = (char *)malloc(6);		
	bzero(mp->magic,6);
	mp->reserve = (char *)malloc(4);
	bzero(mp->reserve,4);
	mp->content = NULL; 
} 
void destory_mmtp(struct mmtp **mp) {
	if((*mp)->magic!=NULL) {
		free((*mp)->magic);
		(*mp)->magic = NULL;
	}
	if((*mp)->reserve != NULL) {
		free((*mp)->reserve);
		(*mp)->reserve = NULL;
	}
	if((*mp)->content != NULL) { 
		free((*mp)->content);
		(*mp)->content = NULL;
	}
}

void mp_clear_close(SOCKET sf_fd,struct mmtp **mp) {
	destory_mmtp(mp);
	close(sf_fd);	
}

int mp_read(SOCKET sf_fd, int *filetype, struct mmtp *mp) {
	int all_read_size = 0;
	while(mp->magic_read_size != 6) { /// read 6 bytes
		int magic_need_read_size = 6-mp->magic_read_size;
		int read_size = dk_read(sf_fd,mp->magic+mp->magic_read_size,magic_need_read_size);	
		if(read_size<=0) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else {
			all_read_size += read_size;
			if(0 == strncmp(mp->magic,"\r\nmmtp",6)) {
				mp->magic_read_size = 6;	
			}else {
				bzero(mp->magic,6);	
				mp->magic_read_size = 0;
			}
		}
	}
	
	if(!mp->has_read_first_and_type) { /// read 1 byte
		char tp_first_byte = 0;
		int read_size = dk_read(sf_fd,&tp_first_byte,1);	
		if(read_size<=0) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else {
			all_read_size += read_size;
			mp->is_first = tp_first_byte & 0x04;
			mp->type = tp_first_byte & 0x03; 		
			mp->has_read_first_and_type = true;
		}
	}

	if(mp->blank_read_size==0) { /// read 1 byte
		char tp_blank_byte = 0;
		int read_size = dk_read(sf_fd,&tp_blank_byte,1);	
		if (read_size<=0) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else  {
			all_read_size += read_size;
			mp->blank_read_size = 1;
		}
	}
	
	while(mp->reserve_read_size != 4) { /// read 4 bytes
		int read_size = dk_read(sf_fd,(char*)mp->reserve+(mp->reserve_read_size),4-mp->reserve_read_size);
		if(read_size<=0) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else {
			all_read_size += read_size;
			mp->reserve_read_size += read_size;
		}
	}

	while(mp->content_length_has_read_size!= 4) { /// read 4 bytes
		int read_size = dk_read(sf_fd,(char*)(&mp->content_length)+(mp->content_length_has_read_size),4-mp->content_length_has_read_size);
		if(read_size<=0) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else {
			all_read_size += read_size;
			mp->content_length_has_read_size+= read_size;
		}
	}
	
	if(mp->content!=NULL){
		free(mp->content);
		mp->content = NULL;
	}
	mp->content = (char*)malloc(mp->content_length);
	bzero(mp->content,mp->content_length);

	while(mp->content_has_read_size!=mp->content_length){
		int read_size =	dk_read(sf_fd,mp->content+mp->content_has_read_size,mp->content_length-mp->content_has_read_size);
		if( read_size <= 0  ) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else {
			all_read_size += read_size;
			mp->content_has_read_size+=read_size;
		}
	}
	return all_read_size;
}	

int mp_write(SOCKET sf_fd, char *data, size_t n, int filetype, bool isfirst) {
	char *content = (char *)malloc(n + 17);	
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

int mp_file_write(SOCKET sf_fd, const char * filename ,int filetype) {
	int fd = open(filename,O_RDONLY);	
	if(fd<0) {
		perror("file open error");
	}
	size_t all_write_size = 0;
	char buffer[file_write_segment_max];
	bzero(buffer, file_write_segment_max);
	size_t read_size = 0;
	while((read_size = read(fd,buffer,file_write_segment_max))) {
		if(read_size > 0) {
			size_t write_size = mp_write(sf_fd,buffer,read_size,filetype,read_size<file_write_segment_max?true:false);
			if(write_size>0) {
				all_write_size += write_size;
			}else if (write_size<=0) {
				perror("write faield");
				break;
			}
		}
	}
	return all_write_size;
}
