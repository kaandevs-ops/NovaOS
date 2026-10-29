#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "nova_files_ui.h"
#include "nova_files_guest_adapter.h"
int main(void){char root[]="/tmp/nova-r50-files-XXXXXX";assert(mkdtemp(root));int fd=open(root,O_RDONLY|O_DIRECTORY);assert(fd>=0);assert(!mkdirat(fd,"Notes",0700));int f=openat(fd,"Türkçe belge.txt",O_CREAT|O_EXCL|O_WRONLY,0600);assert(f>=0);close(f);assert(!symlinkat("/etc",fd,"Unsafe"));struct nova_files_model m;assert(!nova_files_init(&m,fd,&nova_files_guest_ops,0)&&!nova_files_list(&m));struct nova_files_ui ui;nova_files_ui_init(&ui,&m);char path[64];int side=nova_files_ui_sidebar_width(ui.viewport_width);assert(!nova_files_ui_path_at(&ui,side+30,140,path,sizeof path));ui.workspace_shared=1;assert(nova_files_ui_path_at(&ui,side+30,140,path,sizeof path)&&!strcmp(path,"/Notes"));assert(nova_files_ui_path_at(&ui,side+30,180,path,sizeof path)&&!strcmp(path,"/Türkçe belge.txt"));assert(!nova_files_ui_path_at(&ui,side+30,220,path,sizeof path));assert(!nova_files_ui_path_at(&ui,side+30,140,path,4));assert(!nova_files_ui_path_at(&ui,0,140,path,sizeof path));ui.preview_open=1;assert(!nova_files_ui_path_at(&ui,side+30,140,path,sizeof path));nova_files_close(&m);puts("PASS actual Files rows: folder/UTF8 file path, shared-home gate, symlink/toolbar/short-buffer refusal");return 0;}
