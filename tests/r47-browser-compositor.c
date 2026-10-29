#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
#include "../src/compat/nova_browser_bridge.h"
static int clients[NOVA_MAX_CLIENT_CONNECTIONS];
static void pump(void){struct nova_pollfd fds[NOVA_MAX_CLIENT_CONNECTIONS];setup_app_pollfds(fds,0,clients);struct nova_timespec t={0,10000000};nova_poll(fds,NOVA_MAX_CLIENT_CONNECTIONS,&t);consume_app_pollfds(fds,0,clients,1600,900);}
int main(int argc,char**argv){assert(argc==3||argc==4);char path[]="/tmp/nova-r47-native-XXXXXX";assert(mkdtemp(path));
 struct sockaddr_un a={.sun_family=AF_UNIX};snprintf(a.sun_path,sizeof(a.sun_path),"%s/client",path);
 int listener=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK,0);assert(listener>=0);assert(!bind(listener,(struct sockaddr*)&a,sizeof(a))&&!listen(listener,4));
 for(unsigned i=0;i<NOVA_MAX_CLIENT_CONNECTIONS;i++)clients[i]=-1;
 pid_t child=fork();assert(child>=0);if(!child){close(listener);int s=socket(AF_UNIX,SOCK_SEQPACKET,0);assert(!connect(s,(struct sockaddr*)&a,sizeof(a)));int rc=nova_browser_bridge_run(s,argv[1]);close(s);_exit(rc?1:0);}
 for(unsigned n=0;n<100&&!client_connection_count();n++){accept_client_connection(listener,clients);struct timespec t={0,10000000};nanosleep(&t,0);}assert(client_connection_count()==1&&g_connections[0].peer_pid==(unsigned)child);
 int security[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET,0,security));pid_t authority=fork();assert(authority>=0);
 if(!authority){close(security[0]);close(listener);for(unsigned i=0;i<NOVA_MAX_CLIENT_CONNECTIONS;i++)if(clients[i]>=0)close(clients[i]);struct nova_app_security_request q;while(read(security[1],&q,sizeof(q))==sizeof(q)){
  struct nova_app_security_reply r={.magic=NOVA_APP_SECURITY_PROTOCOL_MAGIC,.version=1,.size=sizeof(r),.request_id=q.request_id,.opcode=NOVA_APP_SECURITY_OP_PRINCIPAL,.process_id=q.process_id,.app_generation=1,.policy_generation=1,.client_id=4,.security_flags=NOVA_APP_SECURITY_FLAG_AUTHENTICATED|NOVA_APP_SECURITY_FLAG_POLICY_BOUND,.effective_capabilities=NOVA_APP_CAP_UI_SURFACE|NOVA_APP_CAP_INPUT_TEXT|NOVA_APP_CAP_INPUT_TAB_CAPTURE,.declared_capabilities=NOVA_APP_CAP_UI_SURFACE|NOVA_APP_CAP_INPUT_TEXT|NOVA_APP_CAP_INPUT_TAB_CAPTURE};
  if(q.process_id!=(unsigned)child)r.result=1;memcpy(r.app_id,"nova.browser",13);assert(write(security[1],&r,sizeof(r))==sizeof(r));}_exit(0);}
 close(security[1]);g_security_fd=security[0];
 for(unsigned n=0;n<400&&g_client.commits<1;n++)pump();
 assert(g_connections[0].authenticated&&g_connections[0].client_id==4);struct nova_client_surface*s=find_client_surface(400);assert(s&&s->committed&&g_client.commits>=1);
 struct nova_client_buffer*b=find_client_buffer(4,1);assert(b&&b->map&&b->width==1024&&b->height==768+NOVA_TOPLEVEL_TITLEBAR_HEIGHT);
 FILE*f=fopen(argv[2],"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",b->width,b->height);
 for(unsigned y=0;y<b->height;y++)for(unsigned x=0;x<b->width;x++){unsigned rgb;memcpy(&rgb,(char*)b->map+y*b->stride+x*4,4);unsigned char out[3]={rgb>>16,rgb>>8,rgb};assert(fwrite(out,1,3,f)==3);}assert(!fclose(f));
 if(argc==4){
  if(!strcmp(argv[3],"disconnect")){disconnect_client_connection(&clients[0]);}
  else {
   struct nova_surface_event bad={.magic=NOVA_SURFACE_PROTOCOL_MAGIC,.version=6,.size=sizeof(bad),.type=NOVA_SURFACE_EVENT_FOCUS,.client_id=4,.surface_id=400,.sequence=1,.value=1};
   if(!strcmp(argv[3],"spoof")){bad.client_id=3;assert(write(clients[0],&bad,sizeof(bad))==sizeof(bad));}
   else if(!strcmp(argv[3],"unexpected-fd")){
    int fd=open("/dev/null",O_RDONLY|O_CLOEXEC);assert(fd>=0);char ctrl[CMSG_SPACE(sizeof(int))];memset(ctrl,0,sizeof(ctrl));struct iovec io={&bad,sizeof(bad)};struct msghdr msg={.msg_iov=&io,.msg_iovlen=1,.msg_control=ctrl,.msg_controllen=sizeof(ctrl)};struct cmsghdr*c=CMSG_FIRSTHDR(&msg);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(c),&fd,sizeof(fd));assert(sendmsg(clients[0],&msg,0)==sizeof(bad));close(fd);
   }else assert(0);
  }
  for(unsigned n=0;n<100&&client_connection_count();n++)pump();
  int status;assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==1);
  if(client_connection_count())disconnect_client_connection(&clients[0]);assert(!find_client_surface(400)&&!find_client_buffer(4,1));
  close(g_security_fd);close(listener);assert(waitpid(authority,&status,0)==authority&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
  puts("PASS rejected spoof/unexpected descriptor/disconnect and cleaned production surface resources; fixture authority");return 0;
 }
 s->x=s->pending_x=96;s->y=s->pending_y=84;assert(send_surface_configure(s,1024,768+NOVA_TOPLEVEL_TITLEBAR_HEIGHT));
 for(unsigned n=0;n<20;n++)pump();assert(g_client.configure_acks==1&&!s->awaiting_configure);
 assert(route_client_event(NOVA_SURFACE_EVENT_FOCUS,400,0,1));
 assert(route_client_event(NOVA_SURFACE_EVENT_KEY,400,29,1));
 assert(route_client_event(NOVA_SURFACE_EVENT_FOCUS,400,0,0));
 for(unsigned n=0;n<20;n++)pump();
 assert(route_client_event(NOVA_SURFACE_EVENT_CLOSE_REQUEST,400,0,0));
 for(unsigned n=0;n<100&&client_connection_count();n++)pump();
 int status;assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
 if(client_connection_count())disconnect_client_connection(&clients[0]);assert(!find_client_surface(400)&&!find_client_buffer(4,1));
 close(g_security_fd);close(listener);assert(waitpid(authority,&status,0)==authority&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
 puts("PASS production compositor auth-path, actual browser frame commit, configure/input/focus/close/cleanup; security authority fixture; no DRM/guest");return 0;
}
