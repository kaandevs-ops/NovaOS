#ifndef NOVA_TERMINAL_WORKSPACE_H
#define NOVA_TERMINAL_WORKSPACE_H
#include <linux/loop.h>
#include <sys/vfs.h>
#include <sys/file.h>
#define NOVA_WORKSPACE_LEGACY_BYTES (16UL*1024*1024)
#define NOVA_WORKSPACE_BYTES (256UL*1024*1024)
/* Accept only the legacy and new provisioned capacities, never arbitrary sizes. */
static int workspace_capacity(off_t bytes){return bytes==NOVA_WORKSPACE_LEGACY_BYTES||bytes==NOVA_WORKSPACE_BYTES;}
#ifndef NOVA_WORKSPACE_DATA
#define NOVA_WORKSPACE_DATA "/Data"
#endif
/* Fixed authority path: no client strings, traversal, symlinks or writable
 * ancestors. A fully allocated image bounds user writes independently of
 * boot/update metadata on the outer Data filesystem. */
static int workspace_directory(int fd){
 struct stat s;
 if(fstat(fd,&s)||!S_ISDIR(s.st_mode)||s.st_uid||s.st_gid||(s.st_mode&0022)){errno=EPERM;return -1;}
 return 0;
}
static int workspace_open(void){
 int data=open(NOVA_WORKSPACE_DATA,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(data<0)return errno==ENOENT?-2:-1;
 if(workspace_directory(data)){close(data);return -1;}
 struct statfs fs;
 if(fstatfs(data,&fs)||fs.f_type!=0xef53){close(data);errno=EPERM;return -1;}
 int users=openat(data,"Users",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);close(data);
 if(users<0)return errno==ENOENT?-2:-1;
 if(workspace_directory(users)){close(users);return -1;}
 int fd=openat(users,"nova-workspace.ext4",O_RDWR|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC);close(users);
 if(fd<0)return errno==ENOENT?-2:-1;
 struct stat s;
 if(fstat(fd,&s)||!S_ISREG(s.st_mode)||s.st_uid||s.st_gid||s.st_nlink!=1||(s.st_mode&07777)!=0600||!workspace_capacity(s.st_size)||s.st_blocks< (blkcnt_t)(s.st_size/512)){
  close(fd);errno=EPERM;return -1;
 }
 if(flock(fd,LOCK_EX|LOCK_NB)){close(fd);return -1;}
 return fd;
}
static int workspace_mount(int fd,const char *target){
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
