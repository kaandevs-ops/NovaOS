#define _GNU_SOURCE
#include "nova_browser_profile.h"
/* Trusted boot helper only. No argv paths, environment configuration or IPC. */
int main(int argc,char **argv){
 (void)argv;
 if(argc!=1||getuid()!=0||geteuid()!=0||getppid()!=1)return 126;
 if(close_range(3,~0U,0))return 1;
 int image=browser_profile_open();if(image==-2)return 2;if(image<0)return 1;
 int run=open("/run/nova",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(run<0||browser_profile_directory(run))return 1;
 if(mkdirat(run,"browser-session",0700)&&errno!=EEXIST)return 1;
 int session=openat(run,"browser-session",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);close(run);
 if(session<0||browser_profile_directory(session))return 1;
 struct stat parent;if(fstat(session,&parent)||(parent.st_mode&0077))return 1;
 if(mkdirat(session,"profile",0700)&&errno!=EEXIST)return 1;
 int target=openat(session,"profile",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(target<0||browser_profile_directory(target))return 1;
 struct stat before;if(fstat(target,&before)||before.st_dev!=parent.st_dev)return 1;
 close(target);close(session);
 const char *path="/run/nova/browser-session/profile";
 if(browser_profile_mount(image,path))return 1;
 close(image);
 struct stat profile;struct statfs fs;
 if(stat(path,&profile)||statfs(path,&fs)||profile.st_dev==parent.st_dev||
    profile.st_uid!=4||profile.st_gid!=4||(profile.st_mode&07777)!=0700||
    fs.f_type!=0xef53||(fs.f_flags&(MS_NOSUID|MS_NODEV|MS_NOEXEC))!=(MS_NOSUID|MS_NODEV|MS_NOEXEC)){
  (void)umount(path);return 1;
 }
 puts("[nova-browser-profile] bounded_profile=READY capacity=128MiB owner=4 mode=0700");
 return 0;
}
