#ifndef NOVA_DESKTOP_STORE_H
#define NOVA_DESKTOP_STORE_H
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include "../runtime/nova_terminal_protocol.h"
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
/* Caller runs as workspace user, with an already authorized home directory FD.
 * Never give this write capability to the compositor or read-only Files profile. */
static int nova_desktop_dir(int home){
 struct stat s;
 if(fstat(home,&s)||!S_ISDIR(s.st_mode)||s.st_uid!=geteuid()||(s.st_mode&0022)){errno=EPERM;return -1;}
 if(mkdirat(home,"Desktop",0700)&&errno!=EEXIST)return -1;
 int d=openat(home,"Desktop",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(d<0)return -1;
 if(fstat(d,&s)||!S_ISDIR(s.st_mode)||s.st_uid!=geteuid()||(s.st_mode&0022)){close(d);errno=EPERM;return -1;}
 return d;
}
static int nova_desktop_name(const char*n){
 if(!n||!*n||strlen(n)>255||n[0]=='.')return 0;
 for(const unsigned char*q=(const unsigned char*)n;*q;q++)if(*q<' '||*q==127||*q=='/'||*q=='\\')return 0;
 return 1;
}
/* 0 created and directory synced; 1 created but sync failed; -1 no creation.
 * O_EXCL prevents replacing existing files, symlinks and hardlinks. */
static int nova_desktop_create(int home,const char*name,int directory){
 if(!nova_desktop_name(name)||(directory!=0&&directory!=1)){errno=EINVAL;return -1;}
 int d=nova_desktop_dir(home);if(d<0)return -1;
 int result=-1;
 if(directory){if(!mkdirat(d,name,0700))result=0;}
 else {int f=openat(d,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
  if(f>=0){result=fsync(f)?1:0;if(close(f)&&!result)result=1;}}
 int saved=errno;if(result>=0&&fsync(d))result=1;close(d);errno=saved;return result;
}
static int nova_desktop_snapshot(int home,struct nova_desktop_snapshot*out){
 memset(out,0,sizeof *out);int fd=nova_desktop_dir(home);if(fd<0){out->status=errno;return -1;}
 DIR*d=fdopendir(fd);if(!d){out->status=errno;close(fd);return -1;}struct dirent*e;unsigned seen=0;
 while((e=readdir(d))){if(++seen>256){out->clipped=1;break;}if(!nova_desktop_name(e->d_name))continue;
  if(strlen(e->d_name)>=sizeof out->entries[0].name){out->clipped=1;continue;}
  struct stat st;if(fstatat(dirfd(d),e->d_name,&st,AT_SYMLINK_NOFOLLOW)||(!S_ISREG(st.st_mode)&&!S_ISDIR(st.st_mode)))continue;
  if(out->count==NOVA_DESKTOP_LIMIT){out->clipped=1;break;}
  struct nova_desktop_entry*v=&out->entries[out->count++];v->kind=S_ISDIR(st.st_mode)?2:1;strcpy(v->name,e->d_name);
 }
 closedir(d);
 for(unsigned i=0;i<out->count;i++)for(unsigned j=i+1;j<out->count;j++)if(strcmp(out->entries[i].name,out->entries[j].name)>0){struct nova_desktop_entry t=out->entries[i];out->entries[i]=out->entries[j];out->entries[j]=t;}
 return 0;
}
static int nova_desktop_action(int home,unsigned action,struct nova_desktop_snapshot*out){
 unsigned status=0;if(action<1||action>3){memset(out,0,sizeof *out);out->status=EINVAL;return -1;}
 if(action>1){int result=-1;for(unsigned i=1;i<=999;i++){char name[64];snprintf(name,sizeof name,action==2?"Notlar-%03u.txt":"Klasor-%03u",i);result=nova_desktop_create(home,name,action==3);if(result>=0||errno!=EEXIST)break;}if(result<0)status=errno;else if(result)status=EIO;}
 int rc=nova_desktop_snapshot(home,out);if(status)out->status=status;return status?-1:rc;
}
#endif
