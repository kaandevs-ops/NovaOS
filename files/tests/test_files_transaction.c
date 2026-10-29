#include "nova_files_core.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
struct sim {int mode;int call;int closes;};
static long seek_dir(int fd,void*ctx){(void)fd;((struct sim*)ctx)->call=0;return 0;}
static unsigned int record(unsigned char*b,const char*name,unsigned char dtype){
 unsigned int n=(unsigned int)strlen(name),len=19U+n+1U;
 memset(b,0,len);b[16]=(unsigned char)len;b[17]=(unsigned char)(len>>8U);b[18]=dtype;
 memcpy(b+19,name,n+1U);return len;
}
static long read_dir(int fd,void*out,unsigned long cap,void*ctx){
 (void)fd;struct sim*s=(struct sim*)ctx;
 assert(cap>=256U);
 unsigned char*b=(unsigned char*)out;
 if(s->call++>0)return s->mode==2?-5:0;
 if(s->mode==1){unsigned int n=record(b,"evil",8);b[n+16]=0;return n+19;}
 if(s->mode==3){unsigned int n=record(b,"fake",8);b[n+16]=1;b[n+17]=0;return n+21;}
 unsigned int n=record(b,"zeta",8);n+=record(b+n,"alpha",8);
 n+=record(b+n,"Folder",NOVA_FILES_DTYPE_DIR);
 return (long)n;
}
static long open_dir(int fd,const char*n,void*ctx){(void)fd;(void)n;(void)ctx;return -1;}
static long close_dir(int fd,void*ctx){(void)fd;((struct sim*)ctx)->closes++;return 0;}
int main(void){
 struct nova_files_ops ops={seek_dir,read_dir,open_dir,close_dir};
 struct sim sim={0};struct nova_files_model model;
 assert(nova_files_init(&model,14,&ops,&sim)==NOVA_FILES_OK);
 assert(nova_files_list(&model)==NOVA_FILES_OK);
 assert(model.count==3U&&model.scanned==3U);
 assert(strcmp(model.entries[0].name,"Folder")==0);
 assert(strcmp(model.entries[1].name,"alpha")==0);
 assert(strcmp(model.entries[2].name,"zeta")==0);
 for(int mode=1;mode<=3;mode++){
  sim.mode=mode;
  assert(nova_files_list(&model)==NOVA_FILES_IO);
  assert(model.count==3U&&model.scanned==3U);
  assert(strcmp(model.entries[0].name,"Folder")==0);
 }
 sim.mode=0;assert(nova_files_list(&model)==NOVA_FILES_OK);
 nova_files_close(&model);assert(sim.closes==1);
 puts("R14_ATOMIC_DIRECTORY_SNAPSHOT=PASS");
 puts("R14_MALFORMED_AND_READ_FAILURE=DENIED");
 puts("R14_DETERMINISTIC_SORT=PASS");
 return 0;
}
