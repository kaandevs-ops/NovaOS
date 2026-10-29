#define main unused_browser_main
#include "../src/compat/nova_browser_session.c"
#undef main
#include <assert.h>
static void interrupted(int n){(void)n;}
int main(void){int f[2];assert(!pipe(f));assert(!wait_readable(f[0],20));assert(write(f[1],"x",1)==1);assert(wait_readable(f[0],20));char c;assert(read(f[0],&c,1)==1);struct sigaction a={.sa_handler=interrupted};sigaction(SIGUSR1,&a,0);pid_t child=fork();assert(child>=0);if(!child){usleep(20000);kill(getppid(),SIGUSR1);usleep(20000);assert(write(f[1],"y",1)==1);_exit(0);}assert(wait_readable(f[0],500));assert(read(f[0],&c,1)==1&&c=='y');int st;assert(waitpid(child,&st,0)==child&&WIFEXITED(st)&&!WEXITSTATUS(st));close(f[1]);assert(!wait_readable(f[0],20));close(f[0]);puts("PASS readiness: timeout, readable, EINTR retry, closed peer");return 0;}
