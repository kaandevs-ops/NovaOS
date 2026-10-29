#ifndef NOVA_NET_POLICY_H
#define NOVA_NET_POLICY_H
#include <stddef.h>
#include <linux/filter.h>
#include <linux/audit.h>
#include <linux/seccomp.h>
#include <sys/syscall.h>
#define NALLOW(n) BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_##n,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)
static int net_policy(void){
 struct sock_filter f[]={
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,arch)),
#ifdef __x86_64__
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,AUDIT_ARCH_X86_64,1,0),
#else
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,AUDIT_ARCH_AARCH64,1,0),
#endif
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_KILL_PROCESS),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,nr)),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_openat,0,8),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[2])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,1,0),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[2])),
 BPF_STMT(BPF_ALU|BPF_AND|BPF_K,O_ACCMODE|O_CREAT|O_TRUNC|O_APPEND|020000000),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,1,0),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_socket,0,6),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,3),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,AF_INET,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 NALLOW(read),NALLOW(write),NALLOW(close),NALLOW(fstat),NALLOW(newfstatat),NALLOW(statx),NALLOW(lseek),NALLOW(fcntl),
 NALLOW(connect),NALLOW(sendto),NALLOW(recvfrom),NALLOW(sendmsg),NALLOW(recvmsg),NALLOW(getsockopt),NALLOW(setsockopt),NALLOW(getsockname),NALLOW(getpeername),NALLOW(shutdown),
 NALLOW(brk),NALLOW(mmap),NALLOW(munmap),NALLOW(mprotect),NALLOW(madvise),NALLOW(futex),
 NALLOW(rt_sigaction),NALLOW(rt_sigprocmask),NALLOW(rt_sigreturn),NALLOW(getrandom),NALLOW(getpid),NALLOW(gettid),NALLOW(getuid),NALLOW(geteuid),
 NALLOW(clock_gettime),NALLOW(ppoll),NALLOW(pselect6),NALLOW(setitimer),NALLOW(exit),NALLOW(exit_group),
#ifdef __x86_64__
 NALLOW(poll),NALLOW(select),NALLOW(alarm),NALLOW(stat),NALLOW(lstat),NALLOW(readlink),
#endif
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM)};
 struct sock_fprog p={sizeof f/sizeof f[0],f};return prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&p);
}
#undef NALLOW
#endif
