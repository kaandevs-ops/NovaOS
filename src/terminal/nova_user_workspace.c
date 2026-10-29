#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include "../runtime/nova_terminal_protocol.h"
#include "nova_terminal_workspace.h"
/* PID 1 invokes this verified binary before applications. No listener or
 * client-controlled paths; mount stays in the guest's init mount namespace. */
int main(int argc,char **argv){
 (void)argv;if(argc!=1||getuid()!=0||getppid()!=1)return 126;
 if(close_range(3,~0U,0))return 1;
 int run=open("/run/nova",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(run<0||workspace_directory(run))return 1;
 int image=workspace_open();if(image==-2)return 2;if(image<0)return 1;
 if(mkdirat(run,"user-session",0700)&&errno!=EEXIST)return 1;
 int session=openat(run,"user-session",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);close(run);
 if(session<0||workspace_directory(session))return 1;
 struct stat parent;if(fstat(session,&parent)||(parent.st_mode&0077))return 1;
 if(mkdirat(session,"home",0700)&&errno!=EEXIST)return 1;
 int target=openat(session,"home",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(target<0||workspace_directory(target))return 1;close(target);close(session);
 struct stat backing;if(fstat(image,&backing))return 1;
 if(workspace_mount(image,"/run/nova/user-session/home"))return 1;close(image);
 struct stat home;if(stat("/run/nova/user-session/home",&home)||home.st_uid!=NOVA_TERM_UID||home.st_gid!=NOVA_TERM_UID||(home.st_mode&0777)!=0700){(void)umount("/run/nova/user-session/home");return 1;}
 printf("[nova-user-workspace] shared_home=READY capacity=%ldMiB owner=12004 mode=0700 access=terminal-rw,files-broker-ro\n",(long)(backing.st_size/(1024*1024)));
 return 0;
}
