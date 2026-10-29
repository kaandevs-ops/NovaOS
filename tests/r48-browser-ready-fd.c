#define main unused_browser_main
#include "../src/compat/nova_browser_session.c"
#undef main
#include <assert.h>
int main(int argc,char**argv){if(argc==2&&!strcmp(argv[1],"probe")){if(fcntl(9,F_GETFD)<0)return 7;return write(9,"0\n",2)==2?0:8;}
 for(unsigned i=0;i<2;i++){int p[2];assert(!pipe2(p,O_CLOEXEC));int ready=p[1];if(i){assert(dup2(ready,9)==9);assert(!fcntl(9,F_SETFD,FD_CLOEXEC));close(ready);ready=9;}char*args[]={"/proc/self/exe","probe",NULL};pid_t child=child_exec(args,ready);assert(child>0);close(ready);char b[8]={0};assert(read(p[0],b,sizeof b)==2&&!strcmp(b,"0\n"));close(p[0]);int st;assert(waitpid(child,&st,0)==child&&WIFEXITED(st)&&!WEXITSTATUS(st));}
 puts("PASS X readiness FD survives exec, including ready_fd==9 with CLOEXEC");return 0;}
