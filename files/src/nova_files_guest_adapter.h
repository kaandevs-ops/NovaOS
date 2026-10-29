#ifndef NOVA_FILES_GUEST_ADAPTER_H
#define NOVA_FILES_GUEST_ADAPTER_H
#include "nova_files_core.h"
#include "nova_syscall.h"
/* Freestanding guest backend. Requires separately reviewed privilege grant:
 * delegated directory fd, getdents64/openat2/lseek/close seccomp allowlist.
 * DOES NOT bypass desktop-v1 sandbox or grant filesystem authority. */
#if defined(__x86_64__)
#define NOVA_FILES_NR_GETDENTS64 217L
#define NOVA_FILES_NR_OPENAT2 437L
#elif defined(__aarch64__)
#define NOVA_FILES_NR_GETDENTS64 61L
#define NOVA_FILES_NR_OPENAT2 437L
#else
#error "Unsupported guest target"
#endif
#define NOVA_FILES_O_DIRECTORY 0200000UL
#define NOVA_FILES_RESOLVE_NO_MAGICLINKS 0x02UL
#define NOVA_FILES_RESOLVE_NO_SYMLINKS 0x04UL
#define NOVA_FILES_RESOLVE_BENEATH 0x08UL
struct nova_files_open_how{unsigned long long flags,mode,resolve;};
static long nova_files_guest_seek(int fd,void*ctx){(void)ctx;return nova_lseek(fd,0,NOVA_SEEK_SET);}
static long nova_files_guest_read(int fd,void*buf,unsigned long len,void*ctx){
 (void)ctx;return nova_syscall3(NOVA_FILES_NR_GETDENTS64,fd,(long)buf,(long)len);}
static long nova_files_guest_open(int fd,const char*name,void*ctx){
 (void)ctx;struct nova_files_open_how how;
 how.flags=NOVA_O_RDONLY|NOVA_O_CLOEXEC|NOVA_O_NOFOLLOW|NOVA_FILES_O_DIRECTORY;
 how.mode=0;how.resolve=NOVA_FILES_RESOLVE_BENEATH|NOVA_FILES_RESOLVE_NO_SYMLINKS|NOVA_FILES_RESOLVE_NO_MAGICLINKS;
 return nova_syscall4(NOVA_FILES_NR_OPENAT2,fd,(long)name,(long)&how,sizeof(how));}
static long nova_files_guest_close(int fd,void*ctx){(void)ctx;return nova_close(fd);}
static const struct nova_files_ops nova_files_guest_ops={
 nova_files_guest_seek,nova_files_guest_read,nova_files_guest_open,nova_files_guest_close};
#endif
