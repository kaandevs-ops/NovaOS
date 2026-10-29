#define _GNU_SOURCE
#include "nova_files_core.h"
#include "nova_files_ui.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
extern const struct nova_files_ops nova_files_host_ops;
int main(void){
 char name[]="/tmp/nova-r15-ui-XXXXXX";
 char*root=mkdtemp(name);assert(root);
 char a[512],b[512],c[512];
 snprintf(a,sizeof(a),"%s/Projects",root); assert(mkdir(a,0700)==0);
 snprintf(b,sizeof(b),"%s/alpha.txt",root);
 int f=open(b,O_CREAT|O_EXCL|O_WRONLY|O_CLOEXEC,0600); assert(f>=0);assert(write(f,"x",1)==1);close(f);
 snprintf(c,sizeof(c),"%s/escape",root);assert(symlink("/etc",c)==0);
 int fd=open(root,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);
 struct nova_files_model*m=calloc(1,sizeof(*m));assert(m);
 assert(nova_files_init(m,fd,&nova_files_host_ops,0)==NOVA_FILES_OK);
 assert(nova_files_list(m)==NOVA_FILES_OK);
 struct nova_files_ui u;nova_files_ui_init(&u,m);
 assert(m->count==3);
 assert(m->entries[0].type==NOVA_FILES_DTYPE_DIR);
 /* Stable row hitboxes: row zero enters Projects, row one selects the file. */
 assert(nova_files_ui_pointer(&u,280,145,1,1)==1); assert(m->depth==1);
 assert(strcmp(m->path,"/Projects")==0);
 assert(nova_files_ui_pointer(&u,(int)nova_files_ui_sidebar_width(u.viewport_width)+106,85,1,1)==1);assert(m->depth==0);
 assert(nova_files_ui_key(&u,'r')==1);assert(u.refresh_generation>=3);
 assert(nova_files_ui_pointer(&u,284,185,1,1)==1);assert(m->depth==0);
 /* Refuse selecting symlink as a folder. It is never opened as a directory. */
 assert(nova_files_ui_pointer(&u,284,226,1,1)==1);assert(m->depth==0);
 assert(nova_files_ui_pointer(&u,280,145,2,1)==0);
 assert(nova_files_ui_pointer(&u,-4,145,1,1)==0);
 uint32_t*pix=malloc((size_t)800*500*4+16);assert(pix);
 memset(pix,0x3c,(size_t)800*500*4+16);
 assert(nova_files_ui_render(&u,pix,800,500,3200)==1);
 for(int i=0;i<16;i++) assert(((unsigned char*)pix)[800*500*4+i]==0x3c);
 assert(nova_files_ui_render(&u,pix,801,500,3200)==0);
 assert(nova_files_ui_render(&u,pix,800,500,0)==0);
 assert(nova_files_ui_render(&u,pix,800,500,3)==0);
 unsigned int changed=0;for(int i=0;i<800*500;i++)if(pix[i]!=0x3c3c3c3cU)changed++;
 assert(changed>100000);
 nova_files_close(m);free(m);free(pix);
 assert(unlink(c)==0);assert(unlink(b)==0);assert(rmdir(a)==0);assert(rmdir(root)==0);
 puts("R15_UI_NATIVE_POINTER_NAVIGATION=PASS");
 puts("R15_UI_SYMLINK_NON_TRAVERSAL=PASS");
 puts("R15_UI_PIXEL_GUARDS=PASS");return 0;
}
