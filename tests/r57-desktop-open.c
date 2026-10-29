#define _GNU_SOURCE
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#define nova_main unused_compositor_main
#include "../src/services/compositor.c"
#undef nova_main
int main(void){
 int pair[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK,0,pair));struct nova_client_connection*c=&g_connections[0];c->used=1;c->authenticated=1;c->fd=pair[0];c->client_id=3;c->desktop_open_subscribed=1;c->effective_capabilities=NOVA_APP_CAP_UI_SURFACE;strcpy(c->app_id,"nova.files");
 struct nova_client_surface*s=&g_surfaces[0];s->used=1;s->committed=1;s->owner_fd=pair[0];s->client_id=3;s->surface_id=300;
 strcpy(g_r57_open_name,"Ders");g_r57_open_deadline=nova_render_clock(1)+1000000000ULL;
 int clients[NOVA_MAX_CLIENT_CONNECTIONS];struct nova_pollfd fds[NOVA_MAX_CLIENT_CONNECTIONS];for(unsigned i=0;i<NOVA_MAX_CLIENT_CONNECTIONS;i++)clients[i]=-1;clients[0]=pair[0];
 s->owner_fd=pair[0]+999;setup_app_pollfds(fds,0,clients);assert(!(fds[0].events&4U));s->owner_fd=pair[0];
 c->desktop_open_subscribed=0;r57_flush_open(c);assert(g_r57_open_name[0]);c->desktop_open_subscribed=1;
 c->authenticated=0;r57_flush_open(c);assert(g_r57_open_name[0]);c->authenticated=1;
 strcpy(c->app_id,"nova.demo");r57_flush_open(c);assert(g_r57_open_name[0]);strcpy(c->app_id,"nova.files");
 r57_flush_open(c);assert(!g_r57_open_name[0]);struct nova_surface_event ev;assert(read(pair[1],&ev,sizeof ev)==sizeof ev);assert(nova_desktop_open_event(&ev));assert(!strcmp(ev.clipboard,"Ders"));
 strcpy(g_r57_open_name,"../x");r57_flush_open(c);assert(!g_r57_open_name[0]);assert(read(pair[1],&ev,sizeof ev)<0);
 strcpy(g_r57_open_name,"Ders");g_r57_open_deadline=1;r57_flush_open(c);assert(!g_r57_open_name[0]);
 /* Full desktop press/release double-click routes to Files, not the PTY. */
 int desktop[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET,0,desktop));g_desktop_fd=desktop[0];g_desktop.connected=1;g_r2_canvas_width=1600;g_r2_canvas_height=900;
 g_desktop_files.count=1;g_desktop_files.entries[0].kind=2;strcpy(g_desktop_files.entries[0].name,"Ders");g_desktop_selected=~0U;
 assert(r48_desktop_click(78,90)==1);assert(r49_desktop_drop(78,90)==0);
 assert(r48_desktop_click(78,90)==1);assert(r49_desktop_drop(78,90)==1);
 assert(read(pair[1],&ev,sizeof ev)==sizeof ev);assert(nova_desktop_open_event(&ev));assert(!strcmp(ev.clipboard,"Ders"));
 struct nova_desktop_message dm;assert(read(desktop[1],&dm,sizeof dm)==sizeof dm);assert(dm.client_id==3&&dm.action==NOVA_DESKTOP_ACTION_ACTIVATE);assert(g_terminal.fd<0||!g_terminal.queued);
 /* Bounded retry keeps a request while the Files socket is backpressured. */
 char bytes[136]={0};while(write(pair[0],bytes,sizeof bytes)>0){}
 strcpy(g_r57_open_name,"Ders");g_r57_open_deadline=nova_render_clock(1)+1000000000ULL;r57_flush_open(c);assert(g_r57_open_name[0]);while(read(pair[1],&ev,sizeof ev)>0){}
 r57_flush_open(c);assert(!g_r57_open_name[0]);assert(read(pair[1],&ev,sizeof ev)==sizeof ev&&nova_desktop_open_event(&ev));
 close(desktop[0]);close(desktop[1]);
 close(pair[0]);close(pair[1]);puts("R57_COMPOSITOR_FOLDER_EVENT_PASS");return 0;
}
