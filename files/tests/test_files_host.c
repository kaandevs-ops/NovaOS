#define _GNU_SOURCE
#include "nova_files_core.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
extern const struct nova_files_ops nova_files_host_ops;
static int contains(const struct nova_files_model*m,const char*name){
 for(unsigned int i=0;i<m->count;i++)if(strcmp(name,m->entries[i].name)==0)return 1;
 return 0;}
int main(void){
 char template[]="/tmp/nova-r13-files-XXXXXX";
 char *root=mkdtemp(template);assert(root);
 char folder[256],inner[256],escape[256],afile[256];
 snprintf(folder,sizeof(folder),"%s/Documents",root);
 snprintf(inner,sizeof(inner),"%s/Documents/projects",root);
 snprintf(escape,sizeof(escape),"%s/escape",root);
 snprintf(afile,sizeof(afile),"%s/welcome.txt",root);
 assert(mkdir(folder,0700)==0&&mkdir(inner,0700)==0);
 int f=open(afile,O_WRONLY|O_CREAT|O_EXCL,0600);assert(f>=0);assert(write(f,"hi",2)==2);close(f);
 assert(symlink("/etc",escape)==0);
 int fd=open(root,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(fd>=0);
 struct nova_files_model m;assert(nova_files_init(&m,fd,&nova_files_host_ops,0)==NOVA_FILES_OK);
 assert(nova_files_list(&m)==NOVA_FILES_OK);
 assert(contains(&m,"Documents")&&contains(&m,"welcome.txt")&&contains(&m,"escape"));
 assert(nova_files_enter(&m,"../etc")==NOVA_FILES_ESCAPE);
 assert(nova_files_enter(&m,"/etc")==NOVA_FILES_ESCAPE);
 assert(nova_files_enter(&m,"..")==NOVA_FILES_ESCAPE);
 assert(nova_files_enter(&m,"escape")==NOVA_FILES_IO);
 assert(m.depth==0&&strcmp(m.path,"/")==0);
 assert(nova_files_enter(&m,"welcome.txt")==NOVA_FILES_IO);
 assert(nova_files_enter(&m,"Documents")==NOVA_FILES_OK);
 assert(m.depth==1&&strcmp(m.path,"/Documents")==0&&contains(&m,"projects"));
 assert(nova_files_enter(&m,"projects")==NOVA_FILES_OK);
 assert(m.depth==2&&strcmp(m.path,"/Documents/projects")==0);
 assert(nova_files_parent(&m)==NOVA_FILES_OK&&strcmp(m.path,"/Documents")==0);
 assert(nova_files_parent(&m)==NOVA_FILES_OK&&strcmp(m.path,"/")==0);
 assert(nova_files_parent(&m)==NOVA_FILES_ESCAPE);
 nova_files_close(&m);
 unlink(escape);unlink(afile);rmdir(inner);rmdir(folder);rmdir(root);
 puts("NOVA_FILES_REAL_DIR_ENUMERATION=PASS");
 puts("NOVA_FILES_TRAVERSAL_AND_SYMLINK_DENIAL=PASS");
 puts("NOVA_FILES_BOUNDED_FD_STACK=PASS");
 puts("NOVA_FILES_HOST_TEST=PASS");
 return 0;
}
