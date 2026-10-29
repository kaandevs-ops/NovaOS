#define main nova_browser_bootstrap_main
#include "../src/compat/nova_browser_session.c"
#undef main
#include <assert.h>
static unsigned checks;
static void run(const char*a,const char*b,int expected,long number){
 int fd[2];assert(pipe(fd)==0);pid_t p=fork();assert(p>=0);
 if(!p){close(fd[0]);if(a)assert(write(fd[1],a,strlen(a))==(ssize_t)strlen(a));usleep(15000);if(b)assert(write(fd[1],b,strlen(b))==(ssize_t)strlen(b));close(fd[1]);_exit(0);}
 close(fd[1]);long actual=-1;assert(read_display_number(fd[0],250,&actual)==expected);if(expected)assert(actual==number);close(fd[0]);int status;assert(waitpid(p,&status,0)==p);assert(WIFEXITED(status)&&WEXITSTATUS(status)==0);checks++;
}
int main(void){
 run("0","\n",1,0);run("6","3\n",1,63);run("12\n",NULL,1,12);
 run("\n",NULL,0,0);run("0",NULL,0,0);run("64\n",NULL,0,0);run("-1\n",NULL,0,0);run("0x\n",NULL,0,0);run("100\n",NULL,0,0);run(NULL,NULL,0,0);
 int fd[2];assert(pipe(fd)==0);long actual;assert(!read_display_number(fd[0],20,&actual));close(fd[0]);close(fd[1]);checks++;
 printf("PASS %u real pipe cases: fragmented X display, EOF, malformed, bounds, deadline\n",checks);return 0;
}
