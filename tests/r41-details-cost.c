#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#define nova_main unused_novad_entry
#ifdef NOVA_LEGACY_NOVAD
#include NOVA_LEGACY_NOVAD
#else
#include "../src/init/main.c"
#endif
#undef nova_main
#include "../files/src/nova_files_guest_adapter.h"
#include "../files/src/nova_files_guest_metadata.h"
#ifdef NOVA_LEGACY_CLIENT
#include NOVA_LEGACY_CLIENT
#else
#include "../files/src/nova_files_home_client.h"
#endif

static unsigned long long clock_ns(void){struct nova_timespec t;assert(!nova_clock_gettime(1,&t));return (unsigned long long)t.tv_sec*1000000000ULL+t.tv_nsec;}
static void sort(unsigned long long*a){for(unsigned i=1;i<500;i++){unsigned long long v=a[i];unsigned j=i;while(j&&a[j-1]>v){a[j]=a[j-1];j--;}a[j]=v;}}
static unsigned append_number(char*b,unsigned n,unsigned long long v){char rev[24];unsigned k=0;do{rev[k++]=(char)('0'+v%10);v/=10;}while(v);while(k)b[n++]=rev[--k];return n;}
static unsigned append(char*b,unsigned n,const char*s){while(*s)b[n++]=*s++;return n;}
static void exercise(void){
 assert(nova_close_range(3,~0U,0)>=0&&delegate_home_directory());assert(nova_setgroups(0,0)>=0&&nova_setgid(3)>=0&&nova_setuid(3)>=0);int root=nova_files_receive_home_grant(3);assert(root>=0);nova_close(3);assert(nova_prctl(NOVA_PR_SET_NO_NEW_PRIVS,1,0,0,0)>=0&&sandbox_install_files_seccomp());struct nova_files_metadata meta;assert(nova_files_home_metadata(root,"note.txt",8,&meta)&&meta.posix_valid);
 unsigned long long legacy[500],details[500];
 for(unsigned i=0;i<520;i++)for(unsigned part=0;part<2;part++){
  unsigned which=(part+i)%2;unsigned long long a=clock_ns();
  if(which){assert(nova_files_home_metadata(root,"note.txt",8,&meta)&&meta.posix_valid&&meta.mode==0600&&meta.bytes==12);}
  else{struct nova_home_directory_reply reply;int pass=-1;assert(!home_request(NOVA_HOME_DIRECTORY_META,1,"note.txt",&reply,&pass)&&reply.bytes==12&&pass<0);}
  unsigned long long elapsed=clock_ns()-a;if(i>=20){if(which)details[i-20]=elapsed;else legacy[i-20]=elapsed;}
 }
 sort(legacy);sort(details);char out[400];unsigned n=0;n=append(out,n,"{\"legacy_p50_ns\":");n=append_number(out,n,legacy[249]);n=append(out,n,",\"legacy_p95_ns\":");n=append_number(out,n,legacy[474]);n=append(out,n,",\"details_p50_ns\":");n=append_number(out,n,details[249]);n=append(out,n,",\"details_p95_ns\":");n=append_number(out,n,details[474]);n=append(out,n,"}\n");assert(nova_write(1,out,n)==n);home_guest_close(root,0);assert(home_client.socket<0);nova_exit(0);
}
int main(void){assert(getuid()==0&&!prctl(PR_SET_CHILD_SUBREAPER,1));pid_t child=fork();assert(child>=0);if(!child)exercise();int status;assert(waitpid(child,&status,0)==child&&status==0);unsigned reaped=0;for(unsigned i=0;i<200;i++){int st;pid_t p=waitpid(-1,&st,WNOHANG);if(p>0){assert(st==0);reaped++;continue;}if(p<0&&errno==ECHILD)break;usleep(10000);}assert(reaped==1);}
