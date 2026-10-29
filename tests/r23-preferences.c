#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>
#define NOVA_PREFS_TEST_FAULTS 1
#include "nova_preferences_store.h"
static unsigned int fds(void){DIR*d=opendir("/proc/self/fd");assert(d);unsigned int n=0;while(readdir(d))n++;closedir(d);return n;}
int main(void){char path[]="/tmp/nova-r23-prefs-XXXXXX";assert(mkdtemp(path));int fd=open(path,O_RDONLY|O_DIRECTORY);assert(fd>=0);struct nova_preferences p={2,1},q={0,0};unsigned char b[24];assert(nova_preferences_encode(&p,b)&&nova_preferences_decode(&q,b,24)&&q.theme==2);for(unsigned int i=0;i<24;i++){unsigned char c=b[i];b[i]^=1;assert(!nova_preferences_decode(&q,b,24));b[i]=c;}assert(!nova_preferences_decode(&q,b,23));assert(nova_preferences_load(fd,getuid(),&q)==1);assert(!nova_preferences_store(fd,getuid(),&p)&&!nova_preferences_load(fd,getuid(),&q)&&q.generation==1);assert(nova_preferences_store(fd,getuid(),&p)==-1);p.generation=2;p.theme=1;g_preferences_fail_stage=1;assert(nova_preferences_store(fd,getuid(),&p)==-1&&!nova_preferences_load(fd,getuid(),&q)&&q.generation==1);g_preferences_fail_stage=2;assert(nova_preferences_store(fd,getuid(),&p)==-2&&!nova_preferences_load(fd,getuid(),&q)&&q.generation==2);g_preferences_fail_stage=0;
 unsigned int count=fds();for(unsigned int i=0;i<100;i++){p.generation++;p.theme=i%3;assert(!nova_preferences_store(fd,getuid(),&p)&&!nova_preferences_load(fd,getuid(),&q)&&q.generation==p.generation);}assert(fds()==count);assert(nova_preferences_store(fd,getuid()+1,&p)==-1);assert(!fchmod(fd,0755)&&nova_preferences_load(fd,getuid(),&q)==-1&&!fchmod(fd,0700));
 p.generation++;int barrier[2];assert(!pipe(barrier));pid_t kids[2];for(int i=0;i<2;i++){kids[i]=fork();assert(kids[i]>=0);if(!kids[i]){close(barrier[1]);char c;assert(read(barrier[0],&c,1)==1);_exit(nova_preferences_store(fd,getuid(),&p)==0?0:1);}}close(barrier[0]);assert(write(barrier[1],"xx",2)==2);close(barrier[1]);int successes=0;for(int i=0;i<2;i++){int status;assert(waitpid(kids[i],&status,0)==kids[i]&&WIFEXITED(status));if(WEXITSTATUS(status)==0)successes++;}assert(successes==1&&!nova_preferences_load(fd,getuid(),&q)&&q.generation==p.generation);
 int busy=openat(fd,".appearance.new",O_WRONLY|O_CREAT|O_EXCL,0600);assert(busy>=0);close(busy);p.generation++;assert(nova_preferences_store(fd,getuid(),&p)==-1&&!nova_preferences_load(fd,getuid(),&q)&&q.generation==p.generation-1);close(fd);char links[]="/tmp/nova-r23-prefs-links-XXXXXX";assert(mkdtemp(links));fd=open(links,O_RDONLY|O_DIRECTORY);assert(fd>=0&&!symlinkat("/dev/null",fd,"appearance.v1"));assert(nova_preferences_load(fd,getuid(),&q)==-1&&nova_preferences_store(fd,getuid(),&p)==-1);close(fd);char temp[]="/tmp/nova-r23-prefs-temp-XXXXXX";assert(mkdtemp(temp));fd=open(temp,O_RDONLY|O_DIRECTORY);assert(fd>=0&&!symlinkat("/dev/null",fd,".appearance.new"));assert(nova_preferences_store(fd,getuid(),&p)==-1);close(fd);puts("R23_PREFS_VERSION_CRC_VALIDATION_ATOMIC_FSYNC_FAILURE_UNCERTAIN_REPLAY_OWNER_MODE_BUSY_100_FD_CYCLES=PASS; HOST COMPONENT ONLY, NOT GUEST PERSISTENCE");return 0;}
