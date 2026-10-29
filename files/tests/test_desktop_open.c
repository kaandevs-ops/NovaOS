#define _GNU_SOURCE
#include "nova_files_surface_bridge.h"
#include "nova_desktop_open.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
extern const struct nova_files_ops nova_files_host_ops;
int main(void){
 char dir[]="/tmp/nova-r57-open-XXXXXX";assert(mkdtemp(dir));char path[256];snprintf(path,sizeof path,"%s/Desktop",dir);assert(!mkdir(path,0700));snprintf(path,sizeof path,"%s/Desktop/Ders",dir);assert(!mkdir(path,0700));snprintf(path,sizeof path,"%s/Desktop/Link",dir);assert(!symlink("/tmp",path));
 int fd=open(dir,O_RDONLY|O_DIRECTORY);assert(fd>=0);struct nova_files_model*m=calloc(1,sizeof *m);assert(m);assert(!nova_files_init(m,fd,&nova_files_host_ops,0));assert(!nova_files_list(m));struct nova_files_ui ui;nova_files_ui_init(&ui,m);struct nova_files_surface_bridge b;nova_files_surface_bridge_init(&b,&ui,3,300,0,0);
 struct nova_surface_event e={.magic=NOVA_SURFACE_PROTOCOL_MAGIC,.version=6,.size=sizeof e,.type=NOVA_SURFACE_EVENT_DESKTOP_OPEN,.client_id=3,.surface_id=300,.sequence=1,.clipboard_length=4};memcpy(e.clipboard,"Ders",5);
 ui.preview_open=1;ui.properties_open=1;
 assert(nova_files_surface_bridge_event(&b,&e)==1);assert(!ui.preview_open&&!ui.properties_open);assert(!strcmp(m->path,"/Desktop/Ders"));assert(ui.history_count==2);
 assert(nova_files_surface_bridge_event(&b,&e)==-2);e.sequence++;
 e.client_id=2;assert(nova_files_surface_bridge_event(&b,&e)==-2);e.client_id=3;
 e.clipboard_length=3;assert(nova_files_surface_bridge_event(&b,&e)==-2);e.clipboard_length=4;
 for(unsigned i=0;i<4;i++){const char*bad[]={"../x","a/bx",".abc","a\\bc"};memcpy(e.clipboard,bad[i],5);assert(nova_files_surface_bridge_event(&b,&e)==-2);assert(!strcmp(m->path,"/Desktop/Ders"));}
 memcpy(e.clipboard,"Link",5);assert(nova_files_surface_bridge_event(&b,&e)==-1);assert(!strcmp(m->path,"/Desktop/Ders"));
 e.sequence++;memcpy(e.clipboard,"Gone",5);assert(nova_files_surface_bridge_event(&b,&e)==-1);assert(!strcmp(m->path,"/Desktop/Ders"));
 nova_files_close(m);close(fd);free(m);puts("R57_DESKTOP_OPEN_UI_AND_ESCAPE_PASS");return 0;
}
