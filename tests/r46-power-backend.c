#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#define nova_main nova_init_unused
#include "../src/init/main.c"
#undef nova_main
static int fd_count(void){DIR*d=opendir("/proc/self/fd");assert(d);int n=0;while(readdir(d))n++;closedir(d);return n;}
static int client(void){int fd=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0);assert(fd>=0);struct sockaddr_un a={.sun_family=AF_UNIX};strcpy(a.sun_path,NOVA_POWER_PATH);assert(!connect(fd,(void*)&a,sizeof a));return fd;}
static void send_case(int listener,struct nova_power_message q,int valid){int f=client();assert(send(f,&q,sizeof q,MSG_NOSIGNAL)==sizeof q);poll_power(listener);assert(g_power_pending==valid);assert(g_power_client==-1);close(f);g_power_pending=0;}
int main(void){assert(geteuid()==0);g_service_count=1;strcpy(g_services[0].name,"compositor");g_services[0].pid=getpid();int listener=setup_power_listener();assert(listener>=0);struct nova_power_message q={NOVA_POWER_MAGIC,1,sizeof q,7,1,0};assert(nova_power_valid(&q));
 struct nova_power_message bad=q;bad.reserved=1;send_case(listener,bad,0);bad=q;bad.version=2;send_case(listener,bad,0);bad=q;bad.action=2;send_case(listener,bad,0);bad=q;bad.sequence=0;send_case(listener,bad,0);g_services[0].pid=getpid()+99999;send_case(listener,q,0);g_services[0].pid=getpid();
 int f=client();assert(send(f,&q,sizeof q-1,MSG_NOSIGNAL)==sizeof q-1);poll_power(listener);assert(!g_power_pending&&g_power_client==-1);close(f);
 f=client();poll_power(listener);assert(g_power_client>=0&&!g_power_pending);g_power_deadline=0;poll_power(listener);assert(g_power_client==-1&&!g_power_pending);close(f);
 int passed=memfd_create("r46-rejected-rights",MFD_CLOEXEC);assert(passed>=0);f=client();int before=fd_count();struct iovec iov={&q,sizeof q};union{struct cmsghdr a;unsigned char bytes[CMSG_SPACE(sizeof(int))];}control={0};struct msghdr m={.msg_iov=&iov,.msg_iovlen=1,.msg_control=control.bytes,.msg_controllen=sizeof control};struct cmsghdr*c=CMSG_FIRSTHDR(&m);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(c),&passed,sizeof passed);assert(sendmsg(f,&m,MSG_NOSIGNAL)==sizeof q);poll_power(listener);assert(!g_power_pending&&g_power_client==-1&&fd_count()==before);close(f);close(passed);
 send_case(listener,q,1);assert(getpid()!=1);/* No ordered_shutdown/reboot call is made by this test. */close(listener);unlink(NOVA_POWER_PATH);puts("POWER_REAL_SOCKET_CURRENT_SUPERVISED_PID_BAD_VERSION_ACTION_RESERVED_SHORT_TIMEOUT_PASS_NO_SHUTDOWN_EXECUTED");return 0;}
