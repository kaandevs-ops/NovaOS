#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>
static int r54_fsync_failure;
static int r54_test_fsync(int fd){if(r54_fsync_failure>0&&!--r54_fsync_failure){errno=EIO;return -1;}return fsync(fd);}
#define fsync r54_test_fsync
#include "nova_desktop_files.h"
#include <assert.h>
#include <grp.h>
#include <stdlib.h>
static unsigned checks;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s errno=%d\n",__LINE__,#x,errno);exit(1);}checks++;}while(0)
static void file(int d,const char*n,const void*data,size_t len){int f=openat(d,n,O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW,0600);CHECK(f>=0);CHECK(write(f,data,len)==(ssize_t)len);CHECK(close(f)==0);}
static int fds(void){DIR*d=opendir("/proc/self/fd");CHECK(d);int n=0;while(readdir(d))n++;closedir(d);return n;}
int main(int argc,char**argv){CHECK(argc==2);CHECK(chown(argv[1],12004,12004)==0);CHECK(setgroups(0,0)==0);CHECK(setgid(12004)==0);CHECK(setuid(12004)==0);
 int home=open(argv[1],O_RDONLY|O_DIRECTORY);CHECK(home>=0);int desktop=nova_desktop_dir(home);CHECK(desktop>=0);
 const char text[]="Merhaba NovaOS\nTürkçe: ı İ ç ş ğ ö ü\n";file(desktop,"Not ' $HOME.txt",text,sizeof text-1);
 struct nova_desktop_file_request r={.action=NOVA_DESKTOP_PREVIEW};strcpy(r.name,"Not ' $HOME.txt");struct nova_desktop_file_reply reply;
 CHECK(!nova_desktop_preview(home,&r,&reply));CHECK(reply.length==sizeof text-1&&!memcmp(reply.text,text,sizeof text-1));
 CHECK(!fchmodat(desktop,r.name,0000,0));CHECK(nova_desktop_preview(home,&r,&reply)==-1&&reply.status==EACCES);CHECK(!fchmodat(desktop,r.name,0600,0));
 file(desktop,"Binary", "a\0b",3);strcpy(r.name,"Binary");CHECK(nova_desktop_preview(home,&r,&reply)==-1&&reply.status==ENOTSUP);
 CHECK(!symlinkat("Not ' $HOME.txt",desktop,"Link"));strcpy(r.name,"Link");CHECK(nova_desktop_preview(home,&r,&reply)==-1);
 CHECK(!linkat(desktop,"Not ' $HOME.txt",desktop,"Hard",0));strcpy(r.name,"Hard");CHECK(nova_desktop_preview(home,&r,&reply)==-1);CHECK(!unlinkat(desktop,"Hard",0));
 strcpy(r.name,"../escape");CHECK(!nova_desktop_file_request_valid(&r));CHECK(nova_desktop_preview(home,&r,&reply)==-1&&reply.status==EINVAL);
 unsigned char page[NOVA_DESKTOP_TEXT_BYTES+10];memset(page,'a',sizeof page);page[1799]=0xc4;page[1800]=0xb1;file(desktop,"Paged.txt",page,sizeof page);
 strcpy(r.name,"Paged.txt");r.offset=0;CHECK(!nova_desktop_preview(home,&r,&reply)&&reply.length==1799);r.offset=reply.length;CHECK(!nova_desktop_preview(home,&r,&reply)&&reply.text[0]==0xc4&&reply.text[1]==0xb1);r.offset=1800;CHECK(nova_desktop_preview(home,&r,&reply)==-1&&reply.status==ENOTSUP);
 r.offset=9999;CHECK(nova_desktop_preview(home,&r,&reply)==-1&&reply.status==EINVAL&&reply.offset==0);
 file(desktop,"TooLarge.txt","",0);int big=openat(desktop,"TooLarge.txt",O_WRONLY);CHECK(big>=0&&!ftruncate(big,NOVA_DESKTOP_TEXT_LIMIT+1));close(big);strcpy(r.name,"TooLarge.txt");r.offset=0;CHECK(nova_desktop_preview(home,&r,&reply)==-1);
 CHECK(nova_desktop_text_prefix((const unsigned char*)"\033[31m",5,1)==-1);CHECK(nova_desktop_text_prefix((const unsigned char*)"\xc0\xaf",2,1)==-1);CHECK(nova_desktop_text_prefix((const unsigned char*)"\xed\xa0\x80",3,1)==-1);
 struct stat before,after;CHECK(!fstatat(desktop,"Not ' $HOME.txt",&before,AT_SYMLINK_NOFOLLOW));r.action=NOVA_DESKTOP_TRASH_MOVE;strcpy(r.name,"Not ' $HOME.txt");CHECK(!nova_desktop_trash_change(home,&r,&reply));CHECK(faccessat(desktop,r.name,F_OK,0)==-1&&errno==ENOENT);
 struct nova_desktop_trash_snapshot trash;CHECK(!nova_desktop_trash_snapshot(home,&trash)&&trash.count==1);CHECK(nova_desktop_token_valid(trash.entries[0].token));CHECK(!strcmp(trash.entries[0].name,r.name));
 char token[96];strcpy(token,trash.entries[0].token);int td=nova_desktop_trash_dir(home,desktop);CHECK(td>=0);CHECK(!fstatat(td,token,&after,AT_SYMLINK_NOFOLLOW)&&before.st_ino==after.st_ino);
 file(desktop,r.name,"NEW",3);r.action=NOVA_DESKTOP_TRASH_RESTORE;strcpy(r.name,token);CHECK(nova_desktop_trash_change(home,&r,&reply)==-1&&reply.status==EEXIST);CHECK(!fstatat(td,token,&after,AT_SYMLINK_NOFOLLOW)&&after.st_ino==before.st_ino);
 CHECK(!unlinkat(desktop,"Not ' $HOME.txt",0));CHECK(!nova_desktop_trash_change(home,&r,&reply));CHECK(!fstatat(desktop,"Not ' $HOME.txt",&after,AT_SYMLINK_NOFOLLOW)&&after.st_ino==before.st_ino);
 CHECK(!mkdirat(desktop,"Folder",0700));int folder=openat(desktop,"Folder",O_RDONLY|O_DIRECTORY);CHECK(folder>=0);file(folder,"Child","SAFE",4);close(folder);
 r.action=NOVA_DESKTOP_TRASH_MOVE;strcpy(r.name,"Folder");CHECK(!nova_desktop_trash_change(home,&r,&reply));CHECK(!nova_desktop_trash_snapshot(home,&trash)&&trash.count==1&&trash.entries[0].kind==2);r.action=NOVA_DESKTOP_TRASH_RESTORE;strcpy(r.name,trash.entries[0].token);CHECK(!nova_desktop_trash_change(home,&r,&reply));CHECK(!faccessat(desktop,"Folder/Child",F_OK,0));
 r.action=NOVA_DESKTOP_TRASH_MOVE;strcpy(r.name,"Link");CHECK(nova_desktop_trash_change(home,&r,&reply)==-1&&reply.status==EPERM);
 CHECK(!fchmod(td,0777));strcpy(r.name,"Folder");CHECK(nova_desktop_trash_change(home,&r,&reply)==-1&&reply.status==EPERM);CHECK(!fchmod(td,0700));
 r.action=NOVA_DESKTOP_TRASH_RESTORE;strcpy(r.name,"t-0000000000000000-../escape");CHECK(!nova_desktop_file_request_valid(&r));CHECK(nova_desktop_trash_change(home,&r,&reply)==-1);
 for(unsigned i=0;i<14;i++){char name[32];snprintf(name,sizeof name,"Page-%02u.txt",i);file(desktop,name,"PAGE",4);r.action=NOVA_DESKTOP_TRASH_MOVE;strcpy(r.name,name);CHECK(!nova_desktop_trash_change(home,&r,&reply));}
 CHECK(!nova_desktop_trash_page(home,0,&trash)&&trash.count==12&&trash.total==14&&trash.start==0);char first_token[96];strcpy(first_token,trash.entries[0].token);
 CHECK(!nova_desktop_trash_page(home,12,&trash)&&trash.count==2&&trash.total==14&&trash.start==12);CHECK(strcmp(first_token,trash.entries[0].token));
 CHECK(nova_desktop_trash_page(home,15,&trash)==-1&&trash.status==EINVAL);
 while(1){CHECK(!nova_desktop_trash_snapshot(home,&trash));if(!trash.count)break;for(unsigned i=0;i<trash.count;i++){r.action=NOVA_DESKTOP_TRASH_RESTORE;strcpy(r.name,trash.entries[i].token);CHECK(!nova_desktop_trash_change(home,&r,&reply));}}
 CHECK(!mkfifoat(desktop,"Pipe",0600));r.action=NOVA_DESKTOP_PREVIEW;strcpy(r.name,"Pipe");CHECK(nova_desktop_preview(home,&r,&reply)==-1&&reply.status==ENOTSUP);
 r.action=NOVA_DESKTOP_TRASH_MOVE;strcpy(r.name,"Not ' $HOME.txt");r54_fsync_failure=1;CHECK(nova_desktop_trash_change(home,&r,&reply)==-1&&reply.status==EIO);CHECK(!faccessat(desktop,r.name,F_OK,0));
 r54_fsync_failure=3;CHECK(nova_desktop_trash_change(home,&r,&reply)==-1&&reply.status==EIO);CHECK(faccessat(desktop,r.name,F_OK,0)==-1&&errno==ENOENT);CHECK(!nova_desktop_trash_snapshot(home,&trash)&&trash.count==1);r.action=NOVA_DESKTOP_TRASH_RESTORE;strcpy(r.name,trash.entries[0].token);CHECK(!nova_desktop_trash_change(home,&r,&reply));
 CHECK(!mkdirat(home,"AttackHome",0700));int attack=openat(home,"AttackHome",O_RDONLY|O_DIRECTORY);CHECK(attack>=0);int ad=nova_desktop_dir(attack);CHECK(ad>=0);file(ad,"Preserved.txt","SAFE",4);CHECK(!symlinkat("../Desktop",attack,".nova-trash"));r.action=NOVA_DESKTOP_TRASH_MOVE;strcpy(r.name,"Preserved.txt");CHECK(nova_desktop_trash_change(attack,&r,&reply)==-1);CHECK(!faccessat(ad,"Preserved.txt",F_OK,0));close(ad);close(attack);
 int baseline=fds();for(unsigned i=0;i<200;i++){r.action=NOVA_DESKTOP_TRASH_MOVE;strcpy(r.name,"Not ' $HOME.txt");CHECK(!nova_desktop_trash_change(home,&r,&reply));CHECK(!nova_desktop_trash_snapshot(home,&trash)&&trash.count==1);r.action=NOVA_DESKTOP_TRASH_RESTORE;strcpy(r.name,trash.entries[0].token);CHECK(!nova_desktop_trash_change(home,&r,&reply));r.action=NOVA_DESKTOP_PREVIEW;strcpy(r.name,"Not ' $HOME.txt");CHECK(!nova_desktop_preview(home,&r,&reply));}CHECK(fds()==baseline);
 struct nova_term_packet packet={.magic=NOVA_TERM_MAGIC,.version=1,.op=NOVA_TERM_DESKTOP_FILE,.length=sizeof r};memcpy(packet.data,&r,sizeof r);CHECK(nova_term_valid(&packet,NOVA_TERM_HEADER+sizeof r,1));CHECK(!nova_term_valid(&packet,NOVA_TERM_HEADER+sizeof r-1,1));packet.cols=1;CHECK(!nova_term_valid(&packet,NOVA_TERM_HEADER+sizeof r,1));
 close(td);close(desktop);close(home);printf("PASS %u assertions; real UID12004 text/paging/permissions, reversible trash, no overwrite/traversal/symlink, 200 cycles and FD cleanup\n",checks);return 0;
}
