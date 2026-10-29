#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#define nova_main unused_compositor_main
#include "../src/services/compositor.c"
#undef nova_main
int main(void){int s[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET,0,s));g_desktop_fd=s[0];g_desktop.connected=1;
 struct nova_client_surface*a=&g_surfaces[0],*b=&g_surfaces[1];a->used=a->committed=b->used=b->committed=1;a->client_id=1;a->surface_id=100;a->z_index=10;b->client_id=2;b->surface_id=200;b->z_index=5;
 set_client_focus(100);assert(send_desktop_task_event(2,NOVA_DESKTOP_ACTION_LAUNCH));r48_focus_new_surface(a);assert(g_client.focus_surface==100&&g_launch_focus_client==2);r48_focus_new_surface(b);assert(g_client.focus_surface==200&&b->z_index>a->z_index&&!g_launch_focus_client);
 set_client_focus(100);r48_focus_new_surface(b);assert(g_client.focus_surface==100);
 assert(send_desktop_task_event(2,NOVA_DESKTOP_ACTION_LAUNCH));set_client_focus(100);r48_focus_new_surface(b);assert(g_client.focus_surface==100);
 assert(send_desktop_task_event(2,NOVA_DESKTOP_ACTION_LAUNCH));g_launch_focus_deadline=0;r48_focus_new_surface(b);assert(g_client.focus_surface==100&&!g_launch_focus_client);
 g_terminal.visible=1;g_terminal.focused=0;r48_desktop_activate(4);assert(g_terminal.visible&&g_terminal.focused&&!g_client.focus_surface);close(s[0]);close(s[1]);puts("PASS trusted launch maps focus once, wrong-client/no-intent/manual-cancel/expiry, existing terminal activation");return 0;}
