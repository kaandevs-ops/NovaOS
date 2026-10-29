/* Linux host test adapter ONLY: actual NovaOS runtime uses its own raw
 * syscalls and must not link libc. No host adapter shipped into guest. */
#define _GNU_SOURCE
#include "nova_files_core.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <linux/openat2.h>
#include <errno.h>
#ifndef SYS_openat2
#error "openat2 syscall unavailable on test platform"
#endif
long nova_files_host_seek(int fd,void*ctx){(void)ctx;return lseek(fd,0,SEEK_SET);}
long nova_files_host_read(int fd,void*buf,unsigned long cap,void*ctx){(void)ctx;return syscall(SYS_getdents64,fd,buf,cap);}
long nova_files_host_open(int fd,const char*name,void*ctx){
 (void)ctx;
 struct open_how how={.flags=O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW,
 .resolve=RESOLVE_BENEATH|RESOLVE_NO_SYMLINKS|RESOLVE_NO_MAGICLINKS};
 return syscall(SYS_openat2,fd,name,&how,sizeof(how));
}
long nova_files_host_close(int fd,void*ctx){(void)ctx;return close(fd);}
const struct nova_files_ops nova_files_host_ops={
 nova_files_host_seek,nova_files_host_read,nova_files_host_open,nova_files_host_close};
