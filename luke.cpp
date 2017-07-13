#include "luke.hpp"
#include "dktool.hpp"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <cstdio>
#include <algorithm>
#include <sys/stat.h>
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
    if ((*mp)->options != NULL) {
        free((*mp)->options);
        (*mp)->options = NULL;
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
		ssize_t read_size = dk_read(sf_fd,mp->magic+mp->magic_read_size,magic_need_read_size);
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
		ssize_t read_size = dk_read(sf_fd,&tp_first_byte,1);
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
		ssize_t read_size = dk_read(sf_fd,&tp_blank_byte,1);
		if (read_size<=0) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else  {
			all_read_size += read_size;
			mp->blank_read_size = 1;
		}
	}
	
	while(mp->reserve_read_size != 4) { /// read 4 bytes
		ssize_t read_size = dk_read(sf_fd,(char*)mp->reserve+(mp->reserve_read_size),4-mp->reserve_read_size);
		if(read_size<=0) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else {
			all_read_size += read_size;
			mp->reserve_read_size += read_size;
		}
	}

	while(mp->content_length_has_read_size!= 4) { /// read 4 bytes
		ssize_t read_size = dk_read(sf_fd,(char*)(&mp->content_length)+(mp->content_length_has_read_size),4-mp->content_length_has_read_size);
		if(read_size<=0) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else {
			all_read_size += read_size;
			mp->content_length_has_read_size+= read_size;
		}
	}

	while(mp->option_length_has_read_size != 4) {
		ssize_t read_size = dk_read(sf_fd,(char*)(&mp->option_length)+(mp->option_length_has_read_size),4-mp->option_length_has_read_size);
		if(read_size<=0) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else {
			all_read_size += read_size;
			mp->option_length_has_read_size+= read_size;
		}
	}
	
	if(mp->content!=NULL){
		free(mp->content);
		mp->content = NULL;
	}

	if(mp->options != NULL) {
		free(mp->options);
		mp->options = NULL;
	}

	mp->content = (char*)malloc(mp->content_length);
	bzero(mp->content,mp->content_length);
	if(mp->option_length!=0){
		mp->options = (char *)malloc(mp->option_length+1);
		bzero(mp->options,mp->option_length+1);
	}

	while(mp->content_has_read_size!=mp->content_length){
		ssize_t read_size =	dk_read(sf_fd,mp->content+mp->content_has_read_size,mp->content_length-mp->content_has_read_size);
		if( read_size <= 0  ) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else {
			all_read_size += read_size;
			mp->content_has_read_size+=read_size;
		}
	}

	while(mp->option_has_read_size != mp->option_length) {
		ssize_t read_size =	dk_read(sf_fd,mp->options+mp->option_has_read_size,mp->option_length-mp->option_has_read_size);
		if( read_size <= 0  ) {
			mp_clear_close(sf_fd,&mp);	
			return 0;
		}else {
			all_read_size += read_size;
			mp->option_has_read_size+=read_size;
		}
	}
	if(mp->options == NULL) {
		mp->is_end = true;
	}
	/// check the mmtp struct is an end segment
	else if(strlen(mp->options)>6&&0==strcmp(mp->options+mp->option_length-7,"mmtp\r\n")){
		mp->is_end = true;	
		*(mp->options+mp->option_length-7)='\0';
	} else {
		mp->is_end = false;
	} 
	return all_read_size;
}	

ssize_t mp_write(SOCKET sf_fd, const char *data, size_t n, int filetype, bool isfirst,const char *options) {
	size_t options_size =options==NULL?0:(strlen(options)+1);
	char *content = (char *)malloc(n+20+options_size);
	bzero(content,n+20+options_size);
	strcat(content,"\r\nmmtp");
	char tp_first_byte = 0x00;
	tp_first_byte |= isfirst?0x04:0x00;
	tp_first_byte |= filetype;
	memcpy(content+6,&tp_first_byte,1);
	size_t content_length = n;
	memcpy(content+12,&content_length,4);
	memcpy(content+16,&options_size,4);
	memcpy(content+20,data,n);
	if(options!=NULL) {
		memcpy(content+20+n,options,options_size);
	}
	return 	dk_write(sf_fd, content, n+20+options_size);
}

size_t mp_file_write(SOCKET sf_fd, const char * filename ,int filetype,const char *device_token) {
	if(0 == filetype) {  //// text need special process
        if (device_token != NULL) {
    		char *end_token = (char *)malloc(strlen(device_token)+7);
    		bzero(end_token,strlen(device_token)+7);
    		strcat(end_token,device_token);
    		strcat(end_token,"mmtp\r\n");
    		ssize_t write_size = mp_write(sf_fd,filename,strlen(filename),filetype,true,end_token);
    		return write_size;		
        }else {
    		ssize_t write_size = mp_write(sf_fd,filename,strlen(filename),filetype,true,device_token);
    		return write_size;		
        }
	}

	int fd = open(filename,O_RDONLY);	
	if(fd<0) {
		dk_perror("file open error");
	}
	struct stat st;
	fstat(fd,&st);
	size_t all_write_size = 0;
	char buffer[file_write_segment_max];
	bzero(buffer, file_write_segment_max);
	size_t read_size = 0;
    size_t all_read_size = 0;
	off_t file_size = st.st_size;
	while((read_size = read(fd,buffer,file_write_segment_max))) {
		size_t write_size = 0;
		if(read_size > 0) {
            all_read_size += read_size;
			if(file_size == all_read_size) {
				char *end_token = (char *)malloc(strlen(device_token)+7);
				bzero(end_token,strlen(device_token)+7);
				strcat(end_token,device_token);
				strcat(end_token,"mmtp\r\n");
				write_size = mp_write(sf_fd,buffer,read_size,filetype,all_write_size==0?true:false,end_token);
			}else {
				write_size = mp_write(sf_fd,buffer,read_size,filetype,all_write_size==0?true:false,device_token);
			}
			if(write_size>0) {
				all_write_size += write_size;
			}else if (write_size<=0) {
				dk_perror("write faield");
				break;
			}
		}
	}
	return all_write_size;
}
