#!/usr/bin/env python3
"""Compile the actual PID1 wake-set construction and exercise all authority FDs."""
import pathlib, subprocess, tempfile
r=pathlib.Path(__file__).resolve().parents[1]
s=(r/'src/init/main.c').read_text()
a=s.index('        struct nova_pollfd fds[9];',s.index('poll_overview(overview_listener'))
b=s.index('        if(rc<0',a)
body=s[a:b]
assert 'nova_poll(fds,9,&timeout)' in body
program=r'''
#define _DEFAULT_SOURCE
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <sys/wait.h>
#define NOVA_STDIN -1
#define NOVA_POLLIN POLLIN
#define NOVA_POLLERR POLLERR
#define NOVA_POLLHUP POLLHUP
#define nova_pollfd pollfd
#define nova_timespec timespec
#define nova_zero memset_zero
static void memset_zero(void *p,size_t n){memset(p,0,n);}
static long nova_poll(struct pollfd *p,unsigned n,struct timespec *t){return poll(p,n,(int)(t->tv_sec*1000+t->tv_nsec/1000000));}
int main(void){for(int slot=5;slot<9;slot++){
 int p[2];assert(pipe(p)==0);
 int app_listener=-1,app_client=-1,catalog_listener=-1,catalog_client=-1;
 int g_app_security_listener=-1,security_client=-1,g_resource_security_listener=-1,resource_security_client=-1;
 int *targets[]={&g_app_security_listener,&security_client,&g_resource_security_listener,&resource_security_client};*targets[slot-5]=p[0];
 pid_t child=fork();assert(child>=0);if(child==0){close(p[0]);usleep(10000);assert(write(p[1],"x",1)==1);_exit(0);}close(p[1]);
'''+body+r'''
 assert(rc>0);assert(fds[slot].revents&POLLIN);char c;assert(read(p[0],&c,1)==1&&c=='x');close(p[0]);int status;waitpid(child,&status,0);assert(WIFEXITED(status)&&WEXITSTATUS(status)==0);
 printf("authority wake slot %d PASS\n",slot);
}return 0;}
'''
with tempfile.TemporaryDirectory(prefix='authority-wake-',dir=r/'build/tmp') as d:
    p=pathlib.Path(d);(p/'test.c').write_text(program)
    subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
