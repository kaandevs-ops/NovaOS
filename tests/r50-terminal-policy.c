#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <grp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <stdio.h>
#include "nova_terminal_policy.h"
int main(void){char dir[]="/tmp/nova-r50-policy-XXXXXX";assert(mkdtemp(dir));assert(!chown(dir,12004,12004));assert(!setgroups(0,0)&&!setgid(12004)&&!setuid(12004)&&!chdir(dir));assert(!prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)&&!term_install_policy());assert(umask(077)!=(mode_t)-1);int f=open("private.txt",O_CREAT|O_EXCL|O_WRONLY,0666);assert(f>=0);struct stat st;assert(!fstat(f,&st)&&(st.st_mode&0777)==0600);close(f);errno=0;assert(socket(AF_INET,SOCK_STREAM,0)==-1&&errno==EPERM);errno=0;assert(socket(AF_UNIX,SOCK_STREAM,0)==-1&&errno==EPERM);errno=0;assert(mount("none","/","tmpfs",0,0)==-1&&errno==EPERM);errno=0;assert(unshare(CLONE_NEWUSER)==-1&&errno==EPERM);errno=0;assert(syscall(__NR_ptrace,0,0,0,0)==-1&&errno==EPERM);puts("PASS private umask, no new sockets, mount, user namespace or ptrace");return 0;}
