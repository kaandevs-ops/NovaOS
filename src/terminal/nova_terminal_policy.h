#ifndef NOVA_TERMINAL_POLICY_H
#define NOVA_TERMINAL_POLICY_H
/* Dedicated shell policy. No changes to desktop-v1 or Files policies. */
#include <stddef.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <errno.h>
#include <signal.h>
#include <sched.h>
#define TERM_ALLOW(n) BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_##n,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)
static int term_install_policy(void) {
 struct sock_filter f[]={
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,arch)),
#if defined(__x86_64__)
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,AUDIT_ARCH_X86_64,1,0),
#elif defined(__aarch64__)
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,AUDIT_ARCH_AARCH64,1,0),
#else
#error unsupported terminal architecture
#endif
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_KILL_PROCESS),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,nr)),
 /* fork/exec allowed only inside the already isolated PID/mount/net namespaces. */
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_clone,0,9),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,1,0),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])),
 BPF_STMT(BPF_ALU|BPF_AND|BPF_K,~(unsigned)(CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID)),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,SIGCHLD,2,0),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,SIGCHLD|CLONE_VM|CLONE_VFORK,1,0),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
#ifdef __NR_clone3
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_clone3,0,1),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|ENOSYS),
#endif
 /* Positional I/O uses the same already-confined FD rights as read/write. */
 /* Executable memfd is restricted to CLOEXEC|ALLOW_SEALING|EXEC.
  * The package runner verifies the catalog digest and seals before exec. */
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_memfd_create,0,6),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[1])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,3),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[1])),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0x13,0,1),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 /* R42 atomic create-only save. Same confined paths/UID as renameat;
  * only RENAME_NOREPLACE, never EXCHANGE/WHITEOUT or unknown flags. */
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_renameat2,0,6),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[4])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,3),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[4])),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,1,0,1),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 /* Read-only filesystem statistics stay confined to the existing chroot. */
 TERM_ALLOW(statfs),TERM_ALLOW(fstatfs),
 TERM_ALLOW(pread64),TERM_ALLOW(pwrite64),
 TERM_ALLOW(read),TERM_ALLOW(write),TERM_ALLOW(close),TERM_ALLOW(openat),TERM_ALLOW(lseek),
 TERM_ALLOW(fstat),TERM_ALLOW(newfstatat),TERM_ALLOW(statx),TERM_ALLOW(getdents64),
 TERM_ALLOW(readlinkat),TERM_ALLOW(faccessat),TERM_ALLOW(chdir),TERM_ALLOW(fchdir),TERM_ALLOW(getcwd),
 TERM_ALLOW(unlinkat),TERM_ALLOW(mkdirat),TERM_ALLOW(renameat),TERM_ALLOW(fchmod),TERM_ALLOW(fchmodat),
 /* POSIX ownership metadata: kernel DAC/capability checks still apply after UID drop. */
 TERM_ALLOW(fchown),TERM_ALLOW(fchownat),
 TERM_ALLOW(utimensat),TERM_ALLOW(ftruncate),TERM_ALLOW(fsync),TERM_ALLOW(fdatasync),
 TERM_ALLOW(dup),TERM_ALLOW(dup3),TERM_ALLOW(fcntl),TERM_ALLOW(pipe2),
 TERM_ALLOW(brk),TERM_ALLOW(mmap),TERM_ALLOW(mprotect),TERM_ALLOW(munmap),TERM_ALLOW(mremap),TERM_ALLOW(madvise),
 /* ash background wait uses sigsuspend to atomically unblock SIGCHLD. */
 TERM_ALLOW(rt_sigsuspend),
 TERM_ALLOW(rt_sigaction),TERM_ALLOW(rt_sigprocmask),TERM_ALLOW(rt_sigreturn),TERM_ALLOW(sigaltstack),
 TERM_ALLOW(kill),TERM_ALLOW(tgkill),TERM_ALLOW(getpid),TERM_ALLOW(getppid),TERM_ALLOW(gettid),
 TERM_ALLOW(getuid),TERM_ALLOW(geteuid),TERM_ALLOW(getgid),TERM_ALLOW(getegid),TERM_ALLOW(getgroups),
 TERM_ALLOW(setpgid),TERM_ALLOW(getpgid),TERM_ALLOW(getsid),TERM_ALLOW(setsid),
 TERM_ALLOW(execve),TERM_ALLOW(wait4),TERM_ALLOW(waitid),TERM_ALLOW(exit),TERM_ALLOW(exit_group),
 TERM_ALLOW(ppoll),TERM_ALLOW(pselect6),TERM_ALLOW(clock_gettime),TERM_ALLOW(clock_nanosleep),TERM_ALLOW(nanosleep),
 /* umask changes only the calling session process creation mask. */
 TERM_ALLOW(umask),TERM_ALLOW(uname),TERM_ALLOW(sysinfo),TERM_ALLOW(getrandom),TERM_ALLOW(gettimeofday),TERM_ALLOW(times),
 TERM_ALLOW(futex),TERM_ALLOW(set_tid_address),TERM_ALLOW(set_robust_list),TERM_ALLOW(rseq),TERM_ALLOW(restart_syscall),
#ifdef __NR_arch_prctl
 TERM_ALLOW(arch_prctl),TERM_ALLOW(fork),TERM_ALLOW(vfork),TERM_ALLOW(dup2),TERM_ALLOW(pipe),
 TERM_ALLOW(readlink),TERM_ALLOW(access),TERM_ALLOW(stat),TERM_ALLOW(lstat),TERM_ALLOW(poll),TERM_ALLOW(select),
 TERM_ALLOW(unlink),TERM_ALLOW(mkdir),TERM_ALLOW(rmdir),TERM_ALLOW(rename),TERM_ALLOW(chmod),TERM_ALLOW(getpgrp),
 TERM_ALLOW(chown),TERM_ALLOW(lchown),
 TERM_ALLOW(time),TERM_ALLOW(utime),TERM_ALLOW(utimes),
#endif
 /* Only terminal ioctls. No TIOCSTI input injection, arbitrary device or DRM ioctl. */
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_ioctl,0,18),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[1])),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0x5401,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0x5402,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0x5403,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0x5404,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0x540f,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0x5410,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0x5413,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0x541b,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,nr)),
 /* Unprivileged additional filters only intersect with the current policy.
  * No listener/TSYNC flags, no strict/reset operation, no privilege increase. */
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_seccomp,0,10),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,7),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,SECCOMP_SET_MODE_FILTER,0,5),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[1])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,3),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[1])),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,1),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 /* Limit reads remain compatible. New writes are self (pid=0), CPU only.
  * Unprivileged kernel checks forbid raising the inherited hard ceiling.
  * This lets the PDF child LOWER its CPU hard limit, never AS/FD/other PID. */
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_prlimit64,0,15),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[2])),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,3),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[2])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,1),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,7),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,5),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[1])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,3),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[1])),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,1),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM)
 };
 struct sock_fprog prog={sizeof(f)/sizeof(f[0]),f};
 return prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)<0?-1:prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&prog);
}
#undef TERM_ALLOW
#endif
