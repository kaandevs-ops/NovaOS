#define _GNU_SOURCE
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
int main(int argc,char**argv){
 if(argc!=2||!argv[1][0]||strlen(argv[1])>255||strchr(argv[1],'/')||!strcmp(argv[1],".")||!strcmp(argv[1],"..")){fputs("Usage: nova-sync HOME-FILE (one regular owned file; not global sync)\n",stderr);return 2;}
 if(getuid()!=12004||geteuid()!=12004)return 1;
 int home=open("/home/nova",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(home<0)return 1;
 int fd=openat(home,argv[1],O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);close(home);struct stat st;
 if(fd<0){perror("nova-sync");return 1;}int rc=1;
 if(!fstat(fd,&st)&&S_ISREG(st.st_mode)&&st.st_uid==12004&&st.st_nlink==1){int n;do n=fdatasync(fd);while(n<0&&errno==EINTR);if(!n){puts("FILE_FLUSHED");rc=0;}else perror("nova-sync");}
 close(fd);return rc;
}
