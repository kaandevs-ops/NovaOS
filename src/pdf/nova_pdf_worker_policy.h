#ifndef NOVA_PDF_WORKER_POLICY_H
#define NOVA_PDF_WORKER_POLICY_H
#include <stddef.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <errno.h>
#define PDF_ALLOW(n) BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_##n,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)
static int nova_pdf_worker_policy(void){
 struct sock_filter f[]={
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,arch)),
#if defined(__x86_64__)
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,AUDIT_ARCH_X86_64,1,0),
#elif defined(__aarch64__)
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,AUDIT_ARCH_AARCH64,1,0),
#else
#error unsupported PDF architecture
#endif
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_KILL_PROCESS),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,nr)),
 /* Only the raster-result pipe may be written. Parser messages are silenced. */
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_write,0,6),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,3),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[0])),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,1,0,1),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 /* No executable memory is needed: MuPDF has JavaScript disabled. */
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_mmap,1,0),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_mprotect,0,7),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[2])+4),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,4),
 BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,args[2])),
 BPF_STMT(BPF_ALU|BPF_AND|BPF_K,~3U),
 BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,0,0,1),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
 PDF_ALLOW(close),PDF_ALLOW(brk),PDF_ALLOW(munmap),PDF_ALLOW(mremap),PDF_ALLOW(madvise),
 PDF_ALLOW(futex),PDF_ALLOW(clock_gettime),PDF_ALLOW(rt_sigaction),PDF_ALLOW(rt_sigprocmask),PDF_ALLOW(rt_sigreturn),PDF_ALLOW(getrandom),
 PDF_ALLOW(exit),PDF_ALLOW(exit_group),
 BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM)};
 struct sock_fprog p={sizeof f/sizeof f[0],f};(void)prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0);return (int)syscall(SYS_seccomp,SECCOMP_SET_MODE_FILTER,0,&p);
}
#undef PDF_ALLOW
#endif
