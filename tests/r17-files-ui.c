#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "nova_files_ui.h"
extern const struct nova_files_ops nova_files_linux_ops;
/* Use the production raw guest syscall backend, independent of host adapter symbols. */
#include "nova_files_guest_adapter.h"
int main(void){
 char path[]="/tmp/nova-r17-files-XXXXXX";assert(mkdtemp(path));int root=open(path,O_RDONLY|O_DIRECTORY);assert(root>=0);
 assert(mkdirat(root,"Documents",0700)==0);for(int i=0;i<25;i++){char n[32];snprintf(n,sizeof(n),"Folder-%02d",i);assert(mkdirat(root,n,0700)==0);}
 struct nova_files_model model;assert(nova_files_init(&model,root,&nova_files_guest_ops,0)==0&&nova_files_list(&model)==0);
 struct nova_files_ui ui;nova_files_ui_init(&ui,&model);
 assert(nova_files_ui_key(&ui,'j')==1&&ui.selected==0);assert(nova_files_ui_key(&ui,13)==1&&model.depth==1);
 assert(nova_files_ui_key(&ui,8)==1&&model.depth==0);
 for(int i=0;i<15;i++)assert(nova_files_ui_key(&ui,'j')==1);
 assert(ui.selected==14&&ui.scroll==7);assert(nova_files_ui_resize(&ui,1200,800));
 assert(!nova_files_ui_resize(&ui,639,800)&&!nova_files_ui_resize(&ui,2000,800));
 unsigned long size=1200UL*800UL*4UL;unsigned char*p=malloc(size+32);assert(p);for(unsigned long i=0;i<size+32;i++)p[i]=0x7b;
 assert(nova_files_ui_render(&ui,p,1200,800,4800));for(unsigned int i=0;i<32;i++)assert(p[size+i]==0x7b);
 assert(nova_files_ui_pointer(&ui,300,200,1,1)==1);assert(model.depth==1);
 nova_files_close(&model);free(p);puts("R17_FILES_SELECTION_ENTER_VIEWPORT_DYNAMIC_RENDER_POINTER_GUARDS=PASS");return 0;
}
