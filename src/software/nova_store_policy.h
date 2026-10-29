#ifndef NOVA_STORE_POLICY_H
#define NOVA_STORE_POLICY_H
#include <linux/filter.h>
#include <linux/audit.h>
#include <linux/seccomp.h>
#include <stddef.h>
#define SA(n) BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_##n,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)
static int store_policy(void){struct sock_filter f[]={
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,arch)),
#ifdef __x86_64__
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,AUDIT_ARCH_X86_64,1,0),
#else
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,AUDIT_ARCH_AARCH64,1,0),
#endif
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_KILL_PROCESS),BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,nr)),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_socket,0,9),BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])),BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,AF_INET,1,0),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[1])),BPF_STMT(BPF_ALU|BPF_AND|BPF_K,15),BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,SOCK_STREAM,1,0),BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,SOCK_DGRAM,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_prctl,0,6),BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])),BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,PR_SET_PDEATHSIG,2,0),BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,PR_SET_SECCOMP,1,0),BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,PR_SET_NO_NEW_PRIVS,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_clone3,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|ENOSYS),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_clone,0,4),BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])),BPF_JUMP(BPF_JMP|BPF_JSET|BPF_K,0x7e020000U,1,0),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 SA(read),SA(write),SA(readv),SA(writev),SA(pread64),SA(faccessat),SA(faccessat2),SA(readlinkat),SA(restart_syscall),SA(pipe2),SA(close),SA(close_range),SA(openat),SA(fstat),SA(newfstatat),SA(statx),SA(lseek),SA(fcntl),SA(ftruncate),SA(fsync),SA(fchmod),SA(mkdirat),SA(unlinkat),SA(renameat),SA(renameat2),
 SA(connect),SA(sendto),SA(recvfrom),SA(sendmsg),SA(recvmsg),SA(getsockopt),SA(setsockopt),SA(getsockname),SA(getpeername),SA(shutdown),
 SA(brk),SA(mmap),SA(munmap),SA(mprotect),SA(mremap),SA(madvise),SA(futex),SA(getrandom),SA(memfd_create),
 SA(rt_sigaction),SA(rt_sigprocmask),SA(rt_sigreturn),SA(sigaltstack),SA(getpid),SA(getppid),SA(gettid),SA(getuid),SA(geteuid),SA(getgid),SA(getegid),SA(getgroups),SA(getpgrp),SA(setsid),SA(kill),SA(tgkill),SA(wait4),SA(execve),
 SA(clock_gettime),SA(clock_nanosleep),SA(nanosleep),SA(ppoll),SA(pselect6),SA(setitimer),SA(prlimit64),SA(set_robust_list),SA(set_tid_address),SA(rseq),SA(uname),SA(getcwd),SA(chdir),SA(dup),SA(dup3),SA(exit),SA(exit_group),
#ifdef __x86_64__
 SA(poll),SA(select),SA(access),SA(stat),SA(lstat),SA(readlink),SA(dup2),SA(fork),SA(vfork),SA(arch_prctl),SA(getrlimit),SA(setrlimit),SA(alarm),
#endif
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM)};struct sock_fprog p={sizeof f/sizeof f[0],f};return prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&p);}
#undef SA
#endif
