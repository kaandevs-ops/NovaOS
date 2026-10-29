#define _GNU_SOURCE
#include <sys/vfs.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "nova_terminal_policy.h"
int main(void){
 int fd=open("/",O_RDONLY|O_DIRECTORY);if(fd<0)return 1;
 if(term_install_policy())return 2;
 struct statfs s;if(statfs("/",&s)||fstatfs(fd,&s))return 3;
 errno=0;if(socket(AF_INET,SOCK_STREAM,0)!=-1||errno!=EPERM)return 4;
 errno=0;if(mount("none","/no-such-target","tmpfs",0,NULL)!=-1||errno!=EPERM)return 5;
 errno=0;if(unshare(CLONE_NEWNET)!=-1||errno!=EPERM)return 6;
 puts("READ_ONLY_STATS_PASS_NETWORK_MOUNT_NAMESPACE_STILL_DENIED");return 0;
}
