#define _GNU_SOURCE
#include "nova_pty_host.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
static int valid(unsigned rows, unsigned cols) { return rows && cols && rows <= 512 && cols <= 512; }
int nova_pty_resize(struct nova_pty *p, unsigned rows, unsigned cols) {
    if (!p || p->master < 0 || !valid(rows, cols)) { errno = EINVAL; return -1; }
    struct winsize w = {.ws_row=rows, .ws_col=cols};
    return ioctl(p->master, TIOCSWINSZ, &w);
}
int nova_pty_start(struct nova_pty *p, const char *exe, char *const argv[], unsigned rows, unsigned cols) {
    if (!p || !exe || exe[0]!='/' || !argv || !argv[0] || !valid(rows,cols)) { errno=EINVAL; return -1; }
    p->master=-1; p->child=-1;
    int m=posix_openpt(O_RDWR|O_NOCTTY|O_CLOEXEC);
    if(m<0) return -1;
    char name[128]; int err=ptsname_r(m,name,sizeof name);
    if(err || grantpt(m)<0 || unlockpt(m)<0) { int e=err?err:errno;close(m);errno=e;return -1; }
    int report[2]; if(pipe2(report,O_CLOEXEC)<0) {int e=errno;close(m);errno=e;return -1;}
    pid_t child=fork();
    if(child<0) {int e=errno;close(m);close(report[0]);close(report[1]);errno=e;return -1;}
    if(!child) {
        close(report[0]);close(m);
        int s=-1;
        if(setsid()<0 || (s=open(name,O_RDWR|O_NOCTTY))<0 || ioctl(s,TIOCSCTTY,0)<0) goto fail;
        struct winsize w={.ws_row=rows,.ws_col=cols};
        if(ioctl(s,TIOCSWINSZ,&w)<0) goto fail;
        for(int i=0;i<3;i++) if(dup2(s,i)<0) goto fail;
        if(s>2) close(s);
        /* No inherited host environment or extra descriptors reach executable. */
        if ((report[1]>3 && close_range(3,(unsigned)report[1]-1,0)<0) ||
            close_range((unsigned)report[1]+1,UINT_MAX,0)<0) goto fail;
        char *env[]={"TERM=xterm", "PATH=/usr/bin:/bin", "LANG=C", NULL};
        execve(exe,argv,env);
fail:  {int e=errno; ssize_t sent; do sent=write(report[1],&e,sizeof e);while(sent<0&&errno==EINTR);_exit(127);}
    }
    close(report[1]);int execerr=0;ssize_t n;
    do n=read(report[0],&execerr,sizeof execerr);while(n<0&&errno==EINTR);
    close(report[0]);
    if(n!=0) {close(m);while(waitpid(child,NULL,0)<0&&errno==EINTR){} errno=n>0?execerr:EIO;return -1;}
    p->master=m;p->child=child;return 0;
}
int nova_pty_close(struct nova_pty *p,int *status) {
    if(!p || p->child<=0) {errno=EINVAL;return -1;}
    close(p->master);p->master=-1;
    /* Bounded cleanup; never leave a hung direct child or zombie. */
    (void)kill(-p->child,SIGHUP);(void)kill(p->child,SIGHUP);
    int st=0;pid_t r=0;
    for(int i=0;i<50;i++) {r=waitpid(p->child,&st,WNOHANG);if(r!=0)break;poll(NULL,0,10);}
    if(!r) {(void)kill(-p->child,SIGKILL);(void)kill(p->child,SIGKILL);do r=waitpid(p->child,&st,0);while(r<0&&errno==EINTR);}
    p->child=-1;if(status)*status=st;return r<0?-1:0;
}
