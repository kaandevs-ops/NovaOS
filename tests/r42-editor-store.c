#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../src/terminal/nova_terminal_policy.h"
static int write_fault,sync_fault,sync_calls;
static ssize_t checked_write(int fd,const void*b,size_t n){if(write_fault==2){write_fault=1;return write(fd,b,n>3?3:n);}if(write_fault){errno=ENOSPC;return -1;}return write(fd,b,n);}
static int checked_sync(int fd){if(sync_fault&&++sync_calls==sync_fault){errno=EIO;return -1;}return fsync(fd);}
#define write checked_write
#define fsync checked_sync
#include "../src/terminal/nova_text_store.h"
#undef write
#undef fsync
static int countfd(void){DIR*d=opendir("/proc/self/fd");assert(d);int n=0;while(readdir(d))n++;closedir(d);return n;}
static int temporary(void){DIR*d=opendir(".");assert(d);int n=0;struct dirent*x;while((x=readdir(d)))if(!strncmp(x->d_name,".nova-edit-",11))n++;closedir(d);return n;}
static int exists(const char*n){struct stat st;return !fstatat(AT_FDCWD,n,&st,AT_SYMLINK_NOFOLLOW);}
int main(int argc,char**argv){(void)argv;assert(getuid()==12004&&geteuid()==12004);int root=open(".",O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(root>=0&&!nova_text_root(root));int base=countfd();
 assert(!symlinkat("new.txt",root,"link"));
 if(argc>1){assert(!term_install_policy());assert(syscall(SYS_renameat2,-1,"x",-1,"y",0)==-1&&errno==EPERM);assert(syscall(SYS_renameat2,-1,"x",-1,"y",2)==-1&&errno==EPERM);assert(syscall(SYS_renameat2,-1,"x",-1,"y",4)==-1&&errno==EPERM);assert(syscall(SYS_renameat2,-1,"x",-1,"y",0x100000001ULL)==-1&&errno==EPERM);}
 struct nova_text_document d={0},readback={0};const unsigned char text[]="NovaOS \xc4\xb0\xc4\x9f\xc5\x9f\xc3\xa7\xc3\xb6\xc3\xbc\nreal file\n";assert(!nova_text_load(&d,text,sizeof(text)-1));assert(!nova_text_publish(root,"new.txt",&d));struct stat st;assert(!fstatat(root,"new.txt",&st,0)&&S_ISREG(st.st_mode)&&(st.st_mode&07777)==0600&&st.st_uid==12004&&st.st_nlink==1);assert(!nova_text_read(root,"new.txt",&readback)&&readback.length==d.length&&!memcmp(d.data,readback.data,d.length));
 assert(nova_text_publish(root,"new.txt",&d)<0&&errno==EEXIST);assert(!nova_text_name("../escape")&&!nova_text_name("/absolute")&&!nova_text_name("a/b")&&!nova_text_name("..")&&!nova_text_name(".")&&!nova_text_name(".nova-edit-attack")&&!nova_text_name("a\033b"));
 assert(nova_text_read(root,"link",&readback)<0);assert(nova_text_publish(root,"link",&d)<0&&errno==EEXIST);
 /* Hardlink creation is intentionally done before policy in the unfiltered
  * fixture run. The application has no link creation API. */
 if(argc==1){assert(!linkat(root,"new.txt",root,"hard",0));assert(nova_text_read(root,"hard",&readback)<0&&errno==EPERM);assert(!unlinkat(root,"hard",0));}
 assert(!mkdirat(root,"dir",0700));assert(nova_text_read(root,"dir",&readback)<0);assert(!mkfifoat(root,"fifo",0600)||errno==EPERM);if(exists("fifo"))assert(nova_text_read(root,"fifo",&readback)<0);
 assert(!fchmodat(root,"new.txt",0000,0));assert(nova_text_read(root,"new.txt",&readback)<0&&errno==EACCES);assert(!fchmodat(root,"new.txt",0600,0));
 write_fault=1;assert(nova_text_publish(root,"no-space.txt",&d)<0&&errno==ENOSPC&&!exists("no-space.txt")&&!temporary());write_fault=0;
 write_fault=2;assert(nova_text_publish(root,"partial-space.txt",&d)<0&&errno==ENOSPC&&!exists("partial-space.txt")&&!temporary());write_fault=0;
 sync_fault=1;sync_calls=0;assert(nova_text_publish(root,"no-sync.txt",&d)<0&&errno==EIO&&!exists("no-sync.txt")&&!temporary());
 sync_fault=2;sync_calls=0;assert(nova_text_publish(root,"published-unsynced.txt",&d)==1&&exists("published-unsynced.txt")&&!temporary());sync_fault=0;
 for(unsigned i=0;i<1000;i++)assert(nova_text_publish(root,"new.txt",&d)<0&&errno==EEXIST);assert(countfd()==base&&!temporary());
 if(argc==1){int won=0;for(unsigned i=0;i<8;i++){pid_t p=fork();assert(p>=0);if(!p){struct nova_text_document c={0};unsigned char value='a'+i;assert(!nova_text_insert(&c,&value,1));_exit(nova_text_publish(root,"race.txt",&c)==0?0:errno==EEXIST?1:2);}}for(unsigned i=0;i<8;i++){int status;assert(wait(&status)>0&&WIFEXITED(status));int rc=WEXITSTATUS(status);assert(rc==0||rc==1);won+=rc==0;}assert(won==1&&!temporary());}
 struct nova_text_document u={0};assert(!nova_text_load(&u,(unsigned char*)"a\xc4\xb0\nbc",6));u.cursor=3;nova_text_backspace(&u);assert(u.length==4&&!memcmp(u.data,"a\nbc",4));u.cursor=0;nova_text_right(&u);nova_text_delete(&u);assert(u.length==3&&!memcmp(u.data,"abc",3));
 assert(nova_text_load(&u,(unsigned char*)"\xc0\xaf",2)<0);assert(nova_text_load(&u,(unsigned char*)"\xed\xa0\x80",3)<0);assert(nova_text_load(&u,(unsigned char*)"\x1b[2J",4)<0);
 unsigned char limit[NOVA_TEXT_CAP];memset(limit,'x',sizeof limit);assert(!nova_text_load(&u,limit,sizeof limit));assert(nova_text_insert(&u,(unsigned char*)"y",1)<0&&u.length==NOVA_TEXT_CAP);u.cursor=0;nova_text_backspace(&u);assert(u.length==NOVA_TEXT_CAP);
 unsigned seed=5;u.length=u.cursor=0;for(unsigned i=0;i<20000;i++){seed=seed*1664525U+1013904223U;switch(seed%7){case 0:nova_text_left(&u);break;case 1:nova_text_right(&u);break;case 2:nova_text_backspace(&u);break;case 3:nova_text_delete(&u);break;case 4:(void)nova_text_insert(&u,(unsigned char*)"\xc4\xb0",2);break;case 5:(void)nova_text_insert(&u,(unsigned char*)"\n",1);break;case 6:nova_text_vertical(&u,seed&256);break;}assert(u.cursor<=u.length&&u.length<=NOVA_TEXT_CAP);struct nova_text_document check;assert(!nova_text_load(&check,u.data,u.length));}
 close(root);puts("R42_STORE_PASS real UID12004 atomic create, DAC, UTF8, fault cleanup, 1000FD cycles; policy run restricts rename flags");return 0;
}
