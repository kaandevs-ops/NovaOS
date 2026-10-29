#ifndef NOVA_TEXT_STORE_H
#define NOVA_TEXT_STORE_H
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "nova_text_document.h"
#include "nova_home_file.h"
static int nova_text_read(int root,const char*name,struct nova_text_document*d){
 if(!nova_text_name(name)){errno=EINVAL;return -1;}int fd=openat(root,name,O_RDONLY|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC);if(fd<0)return -1;
 struct stat st;unsigned char b[NOVA_TEXT_CAP+1];size_t used=0;int rc=-1;
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||st.st_uid!=geteuid()){errno=EPERM;goto out;}
 if(st.st_size<0||st.st_size>NOVA_TEXT_CAP){errno=EFBIG;goto out;}
 for(;;){ssize_t n=read(fd,b+used,sizeof b-used);if(n<0&&errno==EINTR)continue;if(n<0)goto out;if(!n)break;used+=(size_t)n;if(used>NOVA_TEXT_CAP){errno=EFBIG;goto out;}}
 if(nova_text_load(d,b,used)){errno=EILSEQ;goto out;}rc=0;
 out:{int saved=errno;close(fd);errno=saved;}return rc;
}
/* 0 durable success; 1 published but directory fsync failed; -1 not published.
 * No replace exists: a concurrent creator, symlink, hardlink or old file wins. */
static int nova_text_publish(int root,const char*name,const struct nova_text_document*d){
 if(!nova_text_name(name)||d->length>NOVA_TEXT_CAP){errno=EINVAL;return -1;}
 unsigned char random[16];size_t got=0;while(got<sizeof random){ssize_t n=getrandom(random+got,sizeof random-got,0);if(n<0&&errno==EINTR)continue;if(n<=0)return -1;got+=(size_t)n;}
 char tmp[48]=".nova-edit-";const char*hex="0123456789abcdef";for(unsigned i=0;i<16;i++){tmp[11+2*i]=hex[random[i]>>4];tmp[12+2*i]=hex[random[i]&15];}tmp[43]=0;
 int fd=openat(root,tmp,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(fd<0)return -1;int error=0;
 if(fchmod(fd,0600))error=errno;
 for(size_t at=0;!error&&at<d->length;){ssize_t n=write(fd,d->data+at,d->length-at);if(n<0&&errno==EINTR)continue;if(n<=0){error=n<0?errno:EIO;break;}at+=(size_t)n;}
 if(!error&&fsync(fd))error=errno;if(close(fd)&&!error)error=errno;
 if(!error&&syscall(SYS_renameat2,root,tmp,root,name,1 /* RENAME_NOREPLACE */))error=errno;
 if(error){(void)unlinkat(root,tmp,0);errno=error;return -1;}
 if(fsync(root))return 1;return 0;
}
#endif
