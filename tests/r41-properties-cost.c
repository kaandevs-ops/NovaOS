#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <time.h>
#include "nova_files_guest_metadata.h"
#include "nova_files_ui.c"
static unsigned long long now(void){struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return (unsigned long long)t.tv_sec*1000000000ULL+t.tv_nsec;}
int main(void){
 char path[]="/tmp/nova-r41-cost-XXXXXX";assert(mkdtemp(path));int dir=open(path,O_RDONLY|O_DIRECTORY);assert(dir>=0);int f=openat(dir,"Permission-demo.txt",O_WRONLY|O_CREAT|O_EXCL,0600);assert(f>=0&&write(f,"Actual file content\n",20)==20&&!fchown(f,12004,12004));close(f);
 struct nova_files_model m;assert(!nova_files_init(&m,dir,&nova_files_guest_ops,0)&&!nova_files_list(&m));struct nova_files_ui u;nova_files_ui_init(&u,&m);nova_files_ui_set_metadata(&u,nova_files_guest_metadata);u.selected=0;u.properties_open=1;unsigned *pixels=calloc(800*500,sizeof *pixels);assert(pixels);unsigned long long values[200];
 for(unsigned i=0;i<220;i++){unsigned long long a=now();assert(nova_files_ui_render(&u,pixels,800,500,800*4));unsigned long long b=now();if(i>=20)values[i-20]=b-a;}
 for(unsigned i=1;i<200;i++){unsigned long long v=values[i];unsigned j=i;while(j&&values[j-1]>v){values[j]=values[j-1];j--;}values[j]=v;}
 struct rusage use;assert(!getrusage(RUSAGE_SELF,&use));printf("{\"p50_ns\":%llu,\"p95_ns\":%llu,\"rss_kib\":%ld,\"metadata_bytes\":%zu,\"ui_bytes\":%zu}\n",values[99],values[189],use.ru_maxrss,sizeof(struct nova_files_metadata),sizeof(struct nova_files_ui));free(pixels);nova_files_close(&m);
}
