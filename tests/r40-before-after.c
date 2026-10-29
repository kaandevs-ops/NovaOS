#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "nova_files_ui.h"
#include "nova_files_guest_adapter.h"
static int cmp(const void*a,const void*b){double aa=*(const double*)a,bb=*(const double*)b;return aa<bb?-1:aa>bb;}
static double now(void){struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return t.tv_sec*1000000.0+t.tv_nsec/1000.0;}
int main(int argc,char**argv){
 assert(argc==5);int fd=open(argv[1],O_RDONLY|O_DIRECTORY);assert(fd>=0);struct nova_files_model m;assert(!nova_files_init(&m,fd,&nova_files_guest_ops,0));assert(!nova_files_list(&m));
 double samples[200];for(unsigned i=0;i<220;i++){double start=now();assert(!nova_files_list(&m));if(i>=20)samples[i-20]=now()-start;}
 qsort(samples,200,sizeof(double),cmp);unsigned loaded=m.count,scanned=m.scanned;
 struct nova_files_ui ui;nova_files_ui_init(&ui,&m);assert(nova_files_ui_find(&ui));for(const char*q=argv[2];*q;q++)assert(nova_files_ui_filter_key(&ui,(unsigned char)*q)>0);
 unsigned matches=nova_files_ui_match_count(&ui);assert(matches==(unsigned)atoi(argv[3]));
 unsigned w=800,h=500;unsigned char*p=calloc((size_t)w*h,4);assert(p&&nova_files_ui_render(&ui,p,w,h,w*4));FILE*f=fopen(argv[4],"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);for(size_t i=0;i<(size_t)w*h;i++){fputc(p[i*4+2],f);fputc(p[i*4+1],f);fputc(p[i*4],f);}assert(!fclose(f));free(p);
 printf("{\"loaded\":%u,\"scanned\":%u,\"matches\":%u,\"list_p50_us\":%.3f,\"list_p95_us\":%.3f,\"model_bytes\":%zu}\n",loaded,scanned,matches,samples[100],samples[190],sizeof(m));nova_files_close(&m);return 0;
}