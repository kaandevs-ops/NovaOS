#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#define NOVA_R47_BROWSER_COMPAT 1
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
static int clients[NOVA_MAX_CLIENT_CONNECTIONS];
static void pump(void){struct nova_pollfd f[NOVA_MAX_CLIENT_CONNECTIONS];setup_app_pollfds(f,0,clients);struct nova_timespec t={0,10000000};nova_poll(f,NOVA_MAX_CLIENT_CONNECTIONS,&t);consume_app_pollfds(f,0,clients,1600,900);}
int main(int argc,char**argv){assert(argc==4);struct sockaddr_un a={.sun_family=AF_UNIX};assert(strlen(argv[2])<sizeof(a.sun_path));strcpy(a.sun_path,argv[2]);
 int listenfd=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK,0);assert(listenfd>=0&&!bind(listenfd,(struct sockaddr*)&a,sizeof(a))&&!chmod(argv[2],0666)&&!listen(listenfd,4));
 for(unsigned i=0;i<NOVA_MAX_CLIENT_CONNECTIONS;i++)clients[i]=-1;
 pid_t child=fork();assert(child>=0);if(!child){close(listenfd);execl(argv[1],argv[1],NULL);_exit(127);}
 int security[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET,0,security));pid_t authority=fork();assert(authority>=0);
 if(!authority){close(security[0]);close(listenfd);struct nova_app_security_request q;while(read(security[1],&q,sizeof(q))==sizeof(q)){
  struct nova_app_security_reply r={.magic=NOVA_APP_SECURITY_PROTOCOL_MAGIC,.version=1,.size=sizeof(r),.request_id=q.request_id,.opcode=NOVA_APP_SECURITY_OP_PRINCIPAL,.process_id=q.process_id,.app_generation=1,.policy_generation=1,.client_id=4,.security_flags=NOVA_APP_SECURITY_FLAG_AUTHENTICATED|NOVA_APP_SECURITY_FLAG_POLICY_BOUND,.effective_capabilities=NOVA_APP_CAP_UI_SURFACE|NOVA_APP_CAP_INPUT_TEXT|NOVA_APP_CAP_INPUT_TAB_CAPTURE,.declared_capabilities=NOVA_APP_CAP_UI_SURFACE|NOVA_APP_CAP_INPUT_TEXT|NOVA_APP_CAP_INPUT_TAB_CAPTURE};
  if(q.process_id!=(unsigned)child)r.result=1;memcpy(r.app_id,"nova.browser",13);assert(write(security[1],&r,sizeof(r))==sizeof(r));}_exit(0);}
 close(security[1]);g_security_fd=security[0];
 for(unsigned i=0;i<2500;i++){if(!client_connection_count())accept_client_connection(listenfd,clients);pump();struct nova_client_surface*s=find_client_surface(400);if(s&&s->committed)break;int st;if(waitpid(child,&st,WNOHANG)==child){fprintf(stderr,"launcher exited status=%d before surface\n",st);return 1;}}
 struct nova_client_surface*s=find_client_surface(400);assert(s&&s->committed&&g_connections[0].authenticated&&g_connections[0].peer_pid==(unsigned)child);
 struct nova_client_buffer*b=find_client_buffer(4,1);assert(b&&b->width==1024&&b->height==650+NOVA_TOPLEVEL_TITLEBAR_HEIGHT);
 for(unsigned i=0;i<1200;i++)pump();
 set_client_focus(400);for(unsigned i=0;i<10;i++)pump();
 g_render.cursor_x=s->x+512;g_render.cursor_y=s->y+NOVA_TOPLEVEL_TITLEBAR_HEIGHT+326;
 assert(route_client_event(NOVA_SURFACE_EVENT_POINTER,400,NOVA_BTN_LEFT,1));assert(route_client_event(NOVA_SURFACE_EVENT_POINTER,400,NOVA_BTN_LEFT,0));
 for(unsigned i=0;i<300;i++)pump();
 g_input_modifiers=NOVA_INPUT_MOD_CTRL;assert(route_client_event(NOVA_SURFACE_EVENT_KEY,400,38,1));assert(route_client_event(NOVA_SURFACE_EVENT_KEY,400,38,0));g_input_modifiers=0;
 unsigned query[]={38,23,49,22,45};for(unsigned i=0;i<5;i++){assert(route_client_event(NOVA_SURFACE_EVENT_KEY,400,query[i],1));assert(route_client_event(NOVA_SURFACE_EVENT_KEY,400,query[i],0));}
 assert(route_client_event(NOVA_SURFACE_EVENT_KEY,400,28,1));assert(route_client_event(NOVA_SURFACE_EVENT_KEY,400,28,0));for(unsigned i=0;i<1000;i++)pump();

 assert(route_client_event(NOVA_SURFACE_EVENT_KEY,400,1,1));assert(route_client_event(NOVA_SURFACE_EVENT_KEY,400,1,0));for(unsigned i=0;i<50;i++)pump();
 FILE*f=fopen(argv[3],"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",b->width,b->height);for(unsigned y=0;y<b->height;y++)for(unsigned x=0;x<b->width;x++){unsigned rgb;memcpy(&rgb,(char*)b->map+y*b->stride+x*4,4);unsigned char bytes[3]={rgb>>16,rgb>>8,rgb};assert(fwrite(bytes,1,3,f)==3);}assert(!fclose(f));
 assert(route_client_event(NOVA_SURFACE_EVENT_CLOSE_REQUEST,400,0,0));
 for(unsigned i=0;i<2000&&client_connection_count();i++)pump();
 int st;assert(waitpid(child,&st,0)==child&&WIFEXITED(st)&&WEXITSTATUS(st)==0);if(client_connection_count())disconnect_client_connection(&clients[0]);assert(!find_client_surface(400)&&!find_client_buffer(4,1));close(g_security_fd);close(listenfd);assert(waitpid(authority,&st,0)==authority&&WIFEXITED(st)&&WEXITSTATUS(st)==0);
 puts("PASS fixed root launcher -> private PID1 Firefox/X -> same supervised PID native bridge -> production compositor; fixture authority, host network, no VM");return 0;
}
