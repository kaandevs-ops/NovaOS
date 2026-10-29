#ifndef NOVA_BROWSER_PROFILE_H
#define NOVA_BROWSER_PROFILE_H
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <linux/loop.h>
#include <sys/vfs.h>
#include <sys/file.h>
#define NOVA_BROWSER_PROFILE_BYTES (128UL*1024*1024)
/* R54: one provisioned capacity; no client-controlled size or path. */
static inline int browser_profile_capacity(off_t bytes){return bytes==NOVA_BROWSER_PROFILE_BYTES;}
#ifndef NOVA_BROWSER_PROFILE_DATA
#define NOVA_BROWSER_PROFILE_DATA "/Data"
#endif
/* Fixed authority path: no client strings, traversal, symlinks or writable
 * ancestors. A fully allocated image bounds user writes independently of
 * boot/update metadata on the outer Data filesystem. */
static inline int browser_profile_directory(int fd){
 struct stat s;
 if(fstat(fd,&s)||!S_ISDIR(s.st_mode)||s.st_uid||s.st_gid||(s.st_mode&0022)){errno=EPERM;return -1;}
 return 0;
}
static inline int browser_profile_open(void){
 int data=open(NOVA_BROWSER_PROFILE_DATA,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(data<0)return errno==ENOENT?-2:-1;
 if(browser_profile_directory(data)){close(data);return -1;}
 struct statfs fs;
 if(fstatfs(data,&fs)||fs.f_type!=0xef53){close(data);errno=EPERM;return -1;}
 int users=openat(data,"Browser",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);close(data);
 if(users<0)return errno==ENOENT?-2:-1;
 if(browser_profile_directory(users)){close(users);return -1;}
 int fd=openat(users,"nova-profile.ext4",O_RDWR|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC);close(users);
 if(fd<0)return errno==ENOENT?-2:-1;
 struct stat s;
 if(fstat(fd,&s)||!S_ISREG(s.st_mode)||s.st_uid||s.st_gid||s.st_nlink!=1||(s.st_mode&07777)!=0600||!browser_profile_capacity(s.st_size)||s.st_blocks< (blkcnt_t)(s.st_size/512)){
  close(fd);errno=EPERM;return -1;
 }
 if(flock(fd,LOCK_EX|LOCK_NB)){close(fd);return -1;}
 return fd;
}
static inline int browser_profile_mount(int fd,const char *target){
 int control=open("/dev/loop-control",O_RDWR|O_CLOEXEC);if(control<0)return -1;
 int loop=-1;
 for(unsigned attempt=0;attempt<8;attempt++){
  int id=ioctl(control,LOOP_CTL_GET_FREE,0);if(id<0)break;
  char path[64];snprintf(path,sizeof path,"/dev/loop%d",id);
  loop=open(path,O_RDWR|O_CLOEXEC);if(loop<0)break;
  struct loop_config config={.fd=(unsigned)fd};config.info.lo_flags=LO_FLAGS_AUTOCLEAR;
  if(ioctl(loop,LOOP_CONFIGURE,&config)==0){
   int result=mount(path,target,"ext4",MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL);
   close(loop);close(control);return result;
  }
  int e=errno;close(loop);loop=-1;if(e!=EBUSY){errno=e;break;}
 }
 close(control);return -1;
}
#endif
