#define _GNU_SOURCE
/* Fixed entry point. Not installed until novad has a dedicated browser runtime
 * launcher/profile. Never use an arbitrary inherited host display or command. */
#include "nova_browser_bridge.h"
#include "nova_surface_protocol.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
int main(int argc,char**argv){
 (void)argv;if(argc!=1||geteuid()==0){fputs("browser bridge requires its supervised private user runtime\n",stderr);return 1;}
 int fd=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0);if(fd<0)return 1;
 struct sockaddr_un a={.sun_family=AF_UNIX};memcpy(a.sun_path,NOVA_SURFACE_SANDBOX_SOCKET_PATH,sizeof(NOVA_SURFACE_SANDBOX_SOCKET_PATH));
 if(connect(fd,(struct sockaddr*)&a,sizeof(a))){perror("private native surface connection");close(fd);return 1;}
 struct ucred peer;socklen_t size=sizeof(peer);
 if(getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&peer,&size)||size!=sizeof(peer)||peer.uid!=0){close(fd);return 1;}
 const char*pid=getenv("NOVA_BROWSER_INIT_PID");char*end=NULL;long init=pid?strtol(pid,&end,10):0;
 struct ucred supervisor;socklen_t sz=sizeof(supervisor);
 if(!pid||!end||*end||init<=1||init>2147483647L||getsockopt(10,SOL_SOCKET,SO_PEERCRED,&supervisor,&sz)||sz!=sizeof(supervisor)||supervisor.uid!=0){close(fd);return 1;}
 const char*display_number=getenv("NOVA_BROWSER_DISPLAY");char*tail=NULL;long number=display_number?strtol(display_number,&tail,10):-1;
 if(!tail||*tail||number<0||number>63){close(fd);close(10);return 1;}char display[32];snprintf(display,sizeof display,"unix/:%ld",number);
 int rc=nova_browser_bridge_run_supervised(fd,display,10);close(fd);(void)send(10,"S",1,MSG_NOSIGNAL);close(10);
 int status;while(waitpid((pid_t)init,&status,0)<0)if(errno!=EINTR)return 1;return rc?1:0;
}
