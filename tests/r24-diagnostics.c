#define _GNU_SOURCE
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#define nova_main unused_init
#include "../src/init/main.c"
#undef nova_main
int main(void){unsigned long long u=0,s=0,rss=0;char stat[1024]="123 (name with ) brackets) S";unsigned int n=0;while(stat[n])n++;for(unsigned int i=4;i<=24;i++)n+=(unsigned int)sprintf(stat+n," %u",i*10);assert(r24_proc_fields(stat,&u,&s,&rss)&&u==140&&s==150&&rss==240);assert(!r24_proc_fields("broken",&u,&s,&rss));assert(!r24_proc_fields("1 (x) S 1 2",&u,&s,&rss));char path[64];r24_proc_path(4294967295U,"stat",path);assert(nova_streq(path,"/proc/4294967295/stat"));unsigned int a=r24_fd_count((unsigned int)getpid());for(int i=0;i<1000;i++)assert(r24_fd_count((unsigned int)getpid())==a);assert(r24_fd_count(4294967295U)==~0U);puts("R24_BOUNDED_PROC_PARSER_PAREN_NAMES_MISSING_DATA_1000_FD_CLEANUP=PASS");}
