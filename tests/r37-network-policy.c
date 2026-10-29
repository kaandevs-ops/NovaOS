#define _GNU_SOURCE
#include <sys/prctl.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <grp.h>
#include "nova_net_policy.h"
int main(void){
 if(setgroups(0,NULL)||setgid(12005)||setuid(12005)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||net_policy())return 1;
 int f=open("/etc/hostname",O_RDONLY);if(f<0)return 2;close(f);
 errno=0;if(open("/tmp/r37-forbidden-created",O_WRONLY|O_CREAT,0600)!=-1||errno!=EPERM)return 3;
 errno=0;if(socket(AF_UNIX,SOCK_STREAM,0)!=-1||errno!=EPERM)return 4;
 int s=socket(AF_INET,SOCK_STREAM,0);if(s<0)return 5;close(s);
 errno=0;if(syscall(__NR_unshare,0x40000000)!=-1||errno!=EPERM)return 6;
 puts("NETWORK_BROKER_READ_ONLY_FILES_INET_ONLY_NO_NAMESPACE_PASS");return 0;
}
