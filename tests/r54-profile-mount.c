#define _GNU_SOURCE
#include "../src/compat/nova_browser_profile.h"
#include <string.h>
int main(int argc,char **argv){
 if(argc!=2)return 2;
 int fd=browser_profile_open();if(fd<0){perror("open");return 3;}
 if(browser_profile_mount(fd,argv[1])){perror("mount");return 4;}
 close(fd);
 struct statfs fs;
 if(statfs(argv[1],&fs)||(fs.f_flags&(MS_NOSUID|MS_NODEV|MS_NOEXEC))!=(MS_NOSUID|MS_NODEV|MS_NOEXEC))return 5;
 char path[4096];if(snprintf(path,sizeof path,"%s/test-preference",argv[1])>=(int)sizeof path)return 6;
 fd=open(path,O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW,0600);
 if(fd<0||write(fd,"Graphite Pro",12)!=12||fsync(fd))return 7;
 close(fd);if(umount(argv[1]))return 8;
 fd=browser_profile_open();if(fd<0||browser_profile_mount(fd,argv[1]))return 9;close(fd);
 fd=open(path,O_RDONLY|O_NOFOLLOW);char text[13]={0};
 if(fd<0||read(fd,text,13)!=12||strcmp(text,"Graphite Pro"))return 10;
 close(fd);if(umount(argv[1]))return 11;
 puts("private namespace ext4 mount/write/fsync/unmount/remount/read PASS");return 0;
}
