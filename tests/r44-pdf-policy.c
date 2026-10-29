#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../src/terminal/nova_terminal_policy.h"
#include "../src/pdf/nova_pdf_worker_policy.h"
int main(void){
 assert(!setgroups(0,NULL));assert(!setgid(12004));assert(!setuid(12004));struct rlimit r={120,120};assert(!setrlimit(RLIMIT_CPU,&r));assert(!term_install_policy());r.rlim_cur=r.rlim_max=3;assert(!setrlimit(RLIMIT_CPU,&r));r.rlim_cur=r.rlim_max=4;errno=0;assert(setrlimit(RLIMIT_CPU,&r)<0&&errno==EPERM);
 assert(syscall(SYS_prlimit64,0,RLIMIT_AS,&r,NULL)<0);assert(syscall(SYS_prlimit64,0,RLIMIT_NOFILE,&r,NULL)<0);assert(syscall(SYS_prlimit64,getpid(),RLIMIT_CPU,&r,NULL)<0);assert(syscall(SYS_prlimit64,1ULL<<32,RLIMIT_CPU,&r,NULL)<0);assert(syscall(SYS_prlimit64,0,1ULL<<32,&r,NULL)<0);
 assert(syscall(SYS_seccomp,SECCOMP_SET_MODE_FILTER,SECCOMP_FILTER_FLAG_NEW_LISTENER,NULL)<0);assert(syscall(SYS_seccomp,1ULL<<32|SECCOMP_SET_MODE_FILTER,0,NULL)<0);assert(syscall(SYS_seccomp,SECCOMP_SET_MODE_FILTER,1ULL<<32,NULL)<0);
 assert(!nova_pdf_worker_policy());assert(open("/etc/passwd",O_RDONLY)<0);assert(socket(AF_INET,SOCK_STREAM,0)<0);assert(fork()<0);assert(write(2,"forbidden",9)<0);assert(mmap(NULL,4096,PROT_READ|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0)==MAP_FAILED);assert(execve("/bin/true",(char*[]){"true",NULL},(char*[]){NULL})<0);
 assert(syscall(SYS_seccomp,SECCOMP_SET_MODE_FILTER,0,NULL)<0);assert(write(1,"R44_STACKED_POLICY_PASS\n",24)==24);return 0;
}
