#define _GNU_SOURCE
#include <unistd.h>
#include <grp.h>
#include <stdio.h>
#include POLICY_HEADER
static void caught(int n){(void)n;}
int main(void){
 struct sigaction a={.sa_handler=caught};sigemptyset(&a.sa_mask);if(sigaction(SIGALRM,&a,0))return 1;
 if(setgroups(0,0)||setgid(12004)||setuid(12004))return 2;
 alarm(1);if(term_install_policy())return 3;
 sigset_t empty;sigemptyset(&empty);errno=0;int rc=sigsuspend(&empty),e=errno;
 printf("real sigsuspend return=%d errno=%d expected=%d\n",rc,e,EXPECT_ERRNO);
 return rc!=-1||e!=EXPECT_ERRNO;
}
