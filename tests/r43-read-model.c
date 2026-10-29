#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/terminal/nova_read_model.h"
static struct nova_read_model doc;
static int fds(void){DIR*d=opendir("/proc/self/fd");assert(d);int n=0;while(readdir(d))n++;closedir(d);return n;}
static void put(int root,const char*name,const void*s,size_t n){int fd=openat(root,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);assert(fd>=0);assert(write(fd,s,n)==(ssize_t)n);assert(!close(fd));}
static unsigned rng=42;
static unsigned random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main(int argc,char**argv){
 assert(argc==2&&getuid()==0);int root=open(argv[1],O_RDONLY|O_DIRECTORY|O_NOFOLLOW);assert(root>=0);assert(!fchown(root,12004,12004)&&!fchmod(root,0700));assert(!setgid(12004)&&!setuid(12004));
 const unsigned char text[]="First\r\nİşletim sistemleri\nLast\n";
 put(root,"notes.txt",text,sizeof text-1);assert(!nova_read_file(root,"notes.txt",&doc));assert(doc.source_bytes==sizeof text-1);assert(strstr((char*)doc.data,"First\n"));assert(!nova_read_layout(&doc,20));assert(doc.count==4);assert(nova_read_find(&doc,"Last",0,2)>=0);assert(doc.top==2);unsigned old=doc.top;assert(nova_read_find(&doc,"missing",0,2)<0&&doc.top==old);assert(nova_read_find(&doc,"First",SIZE_MAX,2)<0);
 nova_read_move(&doc,INT_MIN,2);assert(!doc.top);nova_read_move(&doc,INT_MAX,2);assert(doc.top==2);
 assert(!nova_text_name("../notes.txt")&&!nova_text_name("/notes.txt")&&!nova_text_name("bad\033x"));assert(nova_read_file(root,"../notes.txt",&doc)<0);assert(!symlinkat("notes.txt",root,"link.txt"));assert(nova_read_file(root,"link.txt",&doc)<0);assert(!linkat(root,"notes.txt",root,"hard.txt",0));assert(nova_read_file(root,"notes.txt",&doc)<0);assert(!unlinkat(root,"hard.txt",0));
 assert(!mkfifoat(root,"pipe.txt",0600));assert(nova_read_file(root,"pipe.txt",&doc)<0);assert(!mkdirat(root,"directory",0700));assert(nova_read_file(root,"directory",&doc)<0);
 assert(!fchmodat(root,"notes.txt",0000,0));assert(nova_read_file(root,"notes.txt",&doc)<0&&errno==EACCES);assert(!fchmodat(root,"notes.txt",0600,0));assert(!fchmod(root,0755));assert(nova_read_file(root,"notes.txt",&doc)<0&&errno==EPERM);assert(!fchmod(root,0700));
 const unsigned char bad[][4]={{27,'[','2','J'},{0,0,0,0},{0xc0,0xaf,0,0},{0xed,0xa0,0x80,0},{0xf4,0x90,0x80,0x80},{0xc2,0x9b,0,0},{'\r',0,0,0}};
 for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++)assert(nova_read_load(&doc,bad[i],4)<0);
 unsigned char *large=malloc(NOVA_READ_CAP+1);assert(large);memset(large,'\n',NOVA_READ_CAP+1);assert(!nova_read_load(&doc,large,NOVA_READ_CAP));assert(!nova_read_layout(&doc,20)&&doc.count==NOVA_READ_CAP+1);assert(nova_read_load(&doc,large,NOVA_READ_CAP+1)<0&&errno==EFBIG);put(root,"huge.txt",large,NOVA_READ_CAP+1);assert(nova_read_file(root,"huge.txt",&doc)<0&&errno==EFBIG);free(large);
 int before=fds();for(unsigned i=0;i<1000;i++){assert(!nova_read_file(root,"notes.txt",&doc));assert(nova_read_file(root,"link.txt",&doc)<0);assert(nova_read_file(root,"pipe.txt",&doc)<0);}assert(before==fds());
 unsigned char data[2048];for(unsigned trial=0;trial<20000;trial++){
  unsigned n=random32()%sizeof data;for(unsigned i=0;i<n;i++)data[i]=random32()%9?('a'+random32()%26):'\n';assert(!nova_read_load(&doc,data,n));unsigned width=1+random32()%200;assert(!nova_read_layout(&doc,width));assert(doc.count>=1&&doc.count<=n+1&&doc.line[0]==0);
  for(unsigned i=1;i<doc.count;i++){assert(doc.line[i]>doc.line[i-1]&&doc.line[i]<=n);}
  unsigned rows=1+random32()%44;nova_read_move(&doc,INT_MAX,rows);assert(doc.top<=nova_read_last(&doc,rows));nova_read_reflow(&doc,1+random32()%200,rows);assert(doc.top<doc.count&&doc.top<=nova_read_last(&doc,rows));
 }
 assert(!nova_read_load(&doc,(const unsigned char*)"1234İ56789",11));assert(!nova_read_layout(&doc,5));assert(doc.count==2&&doc.line[1]==6); /* no split UTF-8 */
 assert(!close(root));puts("R43_READ_MODEL_PASS: actual UID/DAC, links/FIFO/traversal/control/size, 1000 stable-FD cycles,20000 layouts,CRLF,UTF8 boundaries");return 0;
}
