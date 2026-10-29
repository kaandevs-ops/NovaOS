#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
int main(void){
 char path[]="/tmp/nova-r16-compositor-XXXXXX";assert(mkdtemp(path));
 struct sockaddr_un addr={.sun_family=AF_UNIX};snprintf(addr.sun_path,sizeof(addr.sun_path),"%s/socket",path);
 int listener=socket(AF_UNIX,SOCK_SEQPACKET,0);assert(listener>=0);
 assert(bind(listener,(struct sockaddr*)&addr,sizeof(addr))==0&&listen(listener,4)==0);
 int clients[NOVA_MAX_CLIENT_CONNECTIONS];for(unsigned int i=0;i<NOVA_MAX_CLIENT_CONNECTIONS;i++)clients[i]=-1;
 pid_t kids[4];
 for(int i=0;i<4;i++){
  kids[i]=fork();assert(kids[i]>=0);
  if(!kids[i]){close(listener);int fd=socket(AF_UNIX,SOCK_SEQPACKET,0);assert(connect(fd,(struct sockaddr*)&addr,sizeof(addr))==0);if(i==2){struct nova_surface_event ev;assert(read(fd,&ev,sizeof(ev))==(long)sizeof(ev));assert(ev.client_id==3&&ev.code==NOVA_KEY_BACKSPACE);}
   else{char c;read(fd,&c,1);}_exit(0);}
  assert(accept_client_connection(listener,clients)==(i<3));
  if(i<3)assert(g_connections[i].peer_pid==(unsigned int)kids[i]);
 }
 int pair[2];assert(socketpair(AF_UNIX,SOCK_SEQPACKET,0,pair)==0);
 pid_t authority=fork();assert(authority>=0);
 if(!authority){
  close(pair[0]);struct nova_app_security_request req;
  while(read(pair[1],&req,sizeof(req))==(long)sizeof(req)){
   struct nova_app_security_reply reply;nova_zero(&reply,sizeof(reply));
   reply.magic=NOVA_APP_SECURITY_PROTOCOL_MAGIC;reply.version=1;reply.size=sizeof(reply);
   reply.request_id=req.request_id;reply.opcode=NOVA_APP_SECURITY_OP_PRINCIPAL;
   reply.result=0;reply.process_id=req.process_id;reply.app_generation=1;reply.policy_generation=1;
   reply.security_flags=NOVA_APP_SECURITY_FLAG_AUTHENTICATED|NOVA_APP_SECURITY_FLAG_POLICY_BOUND;
   reply.declared_capabilities=reply.effective_capabilities=NOVA_APP_CAP_UI_SURFACE;
   for(int i=0;i<3;i++)if(req.process_id==(unsigned int)kids[i])reply.client_id=(unsigned int)i+1;
   if(reply.client_id==3)reply.declared_capabilities=reply.effective_capabilities=NOVA_APP_CAP_UI_SURFACE|NOVA_APP_CAP_INPUT_TEXT;
   nova_copy(reply.app_id,"nova.test",10);
   assert(write(pair[1],&reply,sizeof(reply))==(long)sizeof(reply));
  }
  _exit(0);
 }
 close(pair[1]);g_security_fd=pair[0];
 assert(!authenticate_client_connection(&g_connections[2],2));
 for(unsigned int i=0;i<3;i++)assert(authenticate_client_connection(&g_connections[i],i+1));
 assert(client_id_is_connected(3));
 g_surfaces[0].used=1;g_surfaces[0].client_id=3;g_surfaces[0].surface_id=300;g_surfaces[0].owner_fd=clients[2];
 assert(!route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_TAB,1));
 g_connections[2].effective_capabilities=NOVA_APP_CAP_UI_SURFACE;
 assert(!route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_BACKSPACE,1));
 g_connections[2].effective_capabilities|=NOVA_APP_CAP_INPUT_TEXT;
 assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_BACKSPACE,1));
 struct nova_client_connection duplicate={.used=1,.peer_pid=(unsigned int)kids[0]};
 assert(!authenticate_client_connection(&duplicate,1));
 struct nova_client_connection unknown={.used=1,.peer_pid=(unsigned int)getpid()};
 assert(!authenticate_client_connection(&unknown,3));
 assert(client_buffer_budget_available(3,1600000));assert(!client_buffer_budget_available(3,NOVA_SURFACE_MAX_BUFFER_BYTES+1ULL));
 g_client_buffers[0].used=1;g_client_buffers[0].client_id=3;g_client_buffers[0].bytes=1600000;
 g_client_buffers[0].owner_fd=clients[2];g_client_buffers[0].fd=-1;
 assert(client_buffer_budget_available(3,1));
 g_client_buffers[3].used=1;g_client_buffers[3].client_id=3;g_client_buffers[3].bytes=8000000;
 g_client_buffers[3].owner_fd=clients[2];g_client_buffers[3].fd=-1;
 assert(!client_buffer_budget_available(3,1));g_client_buffers[3].used=0;
 assert(client_buffer_budget_available(1,8*1024*1024));
 g_client_buffers[1].used=1;g_client_buffers[1].client_id=1;g_client_buffers[1].bytes=16*1024*1024;
 g_client_buffers[1].owner_fd=clients[0];g_client_buffers[1].fd=-1;
 assert(!client_buffer_budget_available(1,1));
 g_client_buffers[2].used=1;g_client_buffers[2].client_id=2;g_client_buffers[2].bytes=15*1024*1024;
 g_client_buffers[2].owner_fd=clients[1];g_client_buffers[2].fd=-1;
 assert(!client_buffer_budget_available(2,1024*1024));
 for(int i=0;i<3;i++)disconnect_client_connection(&clients[i]);
 assert(client_connection_count()==0 && !g_client_buffers[0].used && !g_client_buffers[1].used);
 close(g_security_fd);close(listener);
 int status;assert(waitpid(authority,&status,0)==authority&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
 for(int i=0;i<4;i++)assert(waitpid(kids[i],&status,0)==kids[i]&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
 puts("R16_REAL_COMPOSITOR_FUNCTIONS_3_PEERS_AUTH_CAPACITY_BUDGET_CLEANUP=PASS");
 puts("Security authority is mocked; real SO_PEERCRED and production compositor functions executed. DRM/VM not tested.");return 0;
}
