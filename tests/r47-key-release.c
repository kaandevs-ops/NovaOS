#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#define NOVA_R47_BROWSER_COMPAT 1
#define nova_main unused_compositor_main
#include "../src/services/compositor.c"
#undef nova_main
int main(void){int wire[2],input[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK,0,wire));assert(!pipe2(input,O_NONBLOCK));
 g_connections[0].used=1;g_connections[0].authenticated=1;g_connections[0].client_id=4;g_connections[0].fd=wire[0];g_connections[0].effective_capabilities=NOVA_APP_CAP_UI_SURFACE|NOVA_APP_CAP_INPUT_TEXT;
 g_surfaces[0].used=1;g_surfaces[0].committed=1;g_surfaces[0].client_id=4;g_surfaces[0].surface_id=400;g_surfaces[0].owner_fd=wire[0];g_client.focus_surface=400;
 struct nova_input_event keys[2]={{.type=NOVA_EV_KEY,.code=17,.value=1},{.type=NOVA_EV_KEY,.code=17,.value=0}};
 assert(write(input[1],keys,sizeof keys)==sizeof keys);consume_input_fd_impl(input[0],1280,800);
 struct nova_surface_event e;assert(recv(wire[1],&e,sizeof e,0)==sizeof e&&e.type==NOVA_SURFACE_EVENT_KEY&&e.code==17&&e.value==1);
 assert(recv(wire[1],&e,sizeof e,0)==sizeof e&&e.code==17&&e.value==0);
 g_surfaces[0].client_id=3;g_connections[0].client_id=3;assert(write(input[1],&keys[1],sizeof keys[1])==sizeof keys[1]);consume_input_fd_impl(input[0],1280,800);assert(recv(wire[1],&e,sizeof e,0)<0);
 for(int i=0;i<2;i++){close(input[i]);close(wire[i]);}puts("PASS real compositor input loop: browser press+release; legacy client release behavior preserved");return 0;}
