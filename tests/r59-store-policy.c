#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sched.h>
#include <unistd.h>
#include "../src/software/nova_store_policy.h"
int main(void){assert(!prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0));assert(!store_policy());errno=0;assert(socket(AF_INET,SOCK_RAW,1)==-1&&errno==EPERM);errno=0;assert(socket(AF_PACKET,SOCK_RAW,0)==-1&&errno==EPERM);errno=0;assert(unshare(CLONE_NEWUSER)==-1&&errno==EPERM);errno=0;assert(syscall(SYS_ptrace,0,0,0,0)==-1&&errno==EPERM);errno=0;assert(mount(NULL,"/not-a-real-mount-target",NULL,0,NULL)==-1&&errno==EPERM);int fd=socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);assert(fd>=0);close(fd);puts("R59_STORE_SECCOMP_DENIALS_PASS");return 0;}
