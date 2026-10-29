#define _GNU_SOURCE
#include "nova_files_surface_bridge.h"
#include "nova_input.h"
#include <assert.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
extern const struct nova_files_ops nova_files_host_ops;
int main(void){
 char base[]="/tmp/nova-r15-bridge-XXXXXX";char*root=mkdtemp(base);assert(root);
 char p[256];snprintf(p,sizeof(p),"%s/Projects",root);assert(mkdir(p,0700)==0);
 int fd=open(root,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);
 struct nova_files_model*m=calloc(1,sizeof(*m));assert(m);
 assert(nova_files_init(m,fd,&nova_files_host_ops,0)==NOVA_FILES_OK);
 assert(nova_files_list(m)==NOVA_FILES_OK);
 struct nova_files_ui ui;nova_files_ui_init(&ui,m);
 struct nova_files_surface_bridge b;nova_files_surface_bridge_init(&b,&ui,3,300,300,140);
 struct nova_surface_event e={0};e.magic=NOVA_SURFACE_PROTOCOL_MAGIC;
 e.version=NOVA_SURFACE_PROTOCOL_VERSION;e.size=sizeof(e);e.client_id=3;e.surface_id=300;
 e.type=NOVA_SURFACE_EVENT_POINTER;e.code=NOVA_BTN_LEFT;e.value=1;
 e.cursor_x=300+280;e.cursor_y=140+145;
 assert(nova_files_surface_bridge_event(&b,&e)==1);assert(m->depth==1);
 e.cursor_x=INT_MIN;e.cursor_y=INT_MAX;
 assert(nova_files_surface_bridge_event(&b,&e)==0);assert(m->depth==1);
 e.cursor_x=580;e.cursor_y=285;
 e.client_id=2;assert(nova_files_surface_bridge_event(&b,&e)==-2);assert(m->depth==1);
 e.client_id=3;e.version=5;assert(nova_files_surface_bridge_event(&b,&e)==-2);
 e.version=NOVA_SURFACE_PROTOCOL_VERSION;e.size=3;assert(nova_files_surface_bridge_event(&b,&e)==-2);
 assert(b.rejected==3);e.size=sizeof(e);
 e.type=NOVA_SURFACE_EVENT_KEY;e.code=NOVA_KEY_BACKSPACE;
 assert(nova_files_surface_bridge_event(&b,&e)==1);assert(m->depth==0);
 e.type=NOVA_SURFACE_EVENT_CONFIGURE;e.width=400;e.height=400;
 assert(nova_files_surface_bridge_event(&b,&e)==0);assert(b.configured==0);
 e.width=800;e.height=500;e.x=100;e.y=250;
 assert(nova_files_surface_bridge_event(&b,&e)==1);assert(b.surface_x==100&&b.surface_y==250);
 e.type=NOVA_SURFACE_EVENT_CLOSE_REQUEST;
 assert(nova_files_surface_bridge_event(&b,&e)==0&&b.closed==1);
 e.type=NOVA_SURFACE_EVENT_POINTER;e.code=NOVA_BTN_LEFT;
 assert(nova_files_surface_bridge_event(&b,&e)==0);
 e.type=NOVA_SURFACE_EVENT_LAUNCH;
 assert(nova_files_surface_bridge_event(&b,&e)==1&&b.closed==0);
 nova_files_close(m);free(m);assert(rmdir(p)==0);assert(rmdir(root)==0);
 puts("R15_SURFACE_V6_ABI_EVENT_ROUTING=PASS");
 puts("R15_SURFACE_WRONG_CLIENT_AND_INVALID_FRAME=DENIED");
 puts("R15_SURFACE_CLOSE_AND_RELAUNCH_STATE=PASS");return 0;
}
