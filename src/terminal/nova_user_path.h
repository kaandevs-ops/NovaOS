#ifndef NOVA_USER_PATH_H
#define NOVA_USER_PATH_H
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "nova_text_document.h"
/* Read-only home capability walk. No links, parent components or host paths. */
static inline int nova_user_component(const char*s){size_t n=strlen(s);if(!n||n>127||!strcmp(s,".")||!strcmp(s,"..")||!strncmp(s,".nova-edit-",11))return 0;for(size_t i=0;i<n;){unsigned char c=(unsigned char)s[i];if(c<32||c==127||c=='/'||c==92)return 0;unsigned k=nova_text_unit((const unsigned char*)s+i,n-i);if(!k)return 0;i+=k;}return 1;}
static inline int nova_user_parent(int root,const char*home,const char*arg,char name[128]){
 if(!arg||!arg[0]||strlen(arg)>511){errno=EINVAL;return -1;}char path[1024],cwd[512];size_t hn=strlen(home);
 if(arg[0]=='/'){if(strncmp(arg,home,hn)||arg[hn]!='/'){errno=EPERM;return -1;}if(strlen(arg+hn+1)>=sizeof path){errno=ENAMETOOLONG;return -1;}strcpy(path,arg+hn+1);}
 else{if(!getcwd(cwd,sizeof cwd))return -1;if(strncmp(cwd,home,hn)||(cwd[hn]&&cwd[hn]!='/')){errno=EPERM;return -1;}const char*rel=cwd+hn;if(*rel=='/')rel++;size_t n=strlen(rel),a=strlen(arg);if(n+a+2>sizeof path){errno=ENAMETOOLONG;return -1;}memcpy(path,rel,n);if(n)path[n++]='/';memcpy(path+n,arg,a+1);}
 int dir=fcntl(root,F_DUPFD_CLOEXEC,3);if(dir<0)return -1;char*part=path;
 for(;;){char*slash=strchr(part,'/');if(slash)*slash=0;if(!strcmp(part,".")){if(!slash){errno=EINVAL;goto fail;}part=slash+1;continue;}
  if(!nova_user_component(part)){errno=EINVAL;goto fail;}if(!slash){strcpy(name,part);return dir;}
  int next=openat(dir,part,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(next<0)goto fail;struct stat st;
  if(fstat(next,&st)||!S_ISDIR(st.st_mode)||st.st_uid!=geteuid()||st.st_gid!=getegid()||(st.st_mode&0022)){close(next);errno=EPERM;goto fail;}close(dir);dir=next;part=slash+1;
 }
 fail:{int saved=errno;close(dir);errno=saved;return -1;}
}
#endif
