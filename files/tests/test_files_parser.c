#include "nova_files_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct fake {unsigned int turn;unsigned int malform;unsigned int closes;};
static long seek0(int fd,void*ctx){(void)fd;((struct fake*)ctx)->turn=0;return 0;}
static long read0(int fd,void *p,unsigned long cap,void*ctx){
 (void)fd;
 struct fake*f=(struct fake*)ctx;
 if(f->turn++)return 0;
 if(cap<32)return -1;
 unsigned char*b=(unsigned char*)p;memset(b,0,32);
 if(f->malform==1)return 18;
 if(f->malform==2){b[16]=0;b[17]=0;return 24;}
 if(f->malform==3){b[16]=0xff;b[17]=0xff;return 24;}
 if(f->malform==4){b[16]=24;b[17]=0;memset(b+19,'X',5);return 24;}
 b[16]=24;b[17]=0;b[18]=4;b[19]='a';b[20]=0;return 24;
}
static long op(int fd,const char*n,void*ctx){(void)fd;(void)n;(void)ctx;return -1;}
static long cls(int fd,void*ctx){(void)fd;((struct fake*)ctx)->closes++;return 0;}
int main(void){
 struct nova_files_ops ops={seek0,read0,op,cls};
 struct nova_files_model m;struct fake f={0};
 assert(nova_files_init(&m,11,&ops,&f)==NOVA_FILES_OK);
 for(unsigned int i=1;i<=4;i++){f.malform=i;assert(nova_files_list(&m)==NOVA_FILES_IO);}
 f.malform=0;assert(nova_files_list(&m)==NOVA_FILES_OK);
 assert(m.count==1&&strcmp(m.entries[0].name,"a")==0);
 assert(nova_files_enter(&m,"bad/name")==NOVA_FILES_ESCAPE);
 assert(nova_files_enter(&m,"\001ctrl")==NOVA_FILES_ESCAPE);
 assert(nova_files_enter(&m,"\\path")==NOVA_FILES_ESCAPE);
 nova_files_close(&m);assert(f.closes==1);
 puts("NOVA_FILES_MALFORMED_DIRENT=DENIED");
 return 0;
}
