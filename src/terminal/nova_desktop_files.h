#ifndef NOVA_DESKTOP_FILES_H
#define NOVA_DESKTOP_FILES_H
#include "nova_desktop_store.h"
#include <sys/random.h>
#include <sys/syscall.h>
/* Worker is already UID 12004 in its private home namespace. These operations
 * do not grant write access to Files/compositor and never execute user data. */
static int nova_desktop_owned(const struct stat*s){return s->st_uid==geteuid()&&
 (S_ISDIR(s->st_mode)||(S_ISREG(s->st_mode)&&s->st_nlink==1));}
static int nova_desktop_trash_dir(int home,int desktop){
 if(mkdirat(home,".nova-trash",0700)&&errno!=EEXIST)return -1;
 int fd=openat(home,".nova-trash",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(fd<0)return -1;
 struct stat s,d;if(fstat(fd,&s)||fstat(desktop,&d)||s.st_uid!=geteuid()||(s.st_mode&07777)!=0700||s.st_dev!=d.st_dev){close(fd);errno=EPERM;return -1;}
 return fd;
}
static int nova_desktop_preview(int home,const struct nova_desktop_file_request*r,struct nova_desktop_file_reply*out){
 memset(out,0,sizeof *out);out->action=r->action;
 if(!nova_desktop_file_request_valid(r)||r->action!=NOVA_DESKTOP_PREVIEW){errno=EINVAL;goto fail;}
 strcpy(out->name,r->name);out->offset=r->offset;
 int d=nova_desktop_dir(home);if(d<0)goto fail;
 struct stat prior;if(fstatat(d,r->name,&prior,AT_SYMLINK_NOFOLLOW)){int e=errno;close(d);errno=e;goto fail;}
 if(!S_ISREG(prior.st_mode)||!nova_desktop_owned(&prior)){close(d);errno=ENOTSUP;goto fail;}
 int f=openat(d,r->name,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);int saved=errno;close(d);errno=saved;if(f<0)goto fail;
 struct stat s;int ok=fstat(f,&s)==0&&S_ISREG(s.st_mode)&&nova_desktop_owned(&s)&&s.st_dev==prior.st_dev&&s.st_ino==prior.st_ino&&s.st_size>=0&&s.st_size<=NOVA_DESKTOP_TEXT_LIMIT;
 if(!ok){close(f);errno=ENOTSUP;goto fail;}
 out->total=(unsigned)s.st_size;
 if(r->offset>out->total){close(f);errno=EINVAL;goto fail;}
 ssize_t n=pread(f,out->text,sizeof out->text,r->offset);saved=errno;close(f);errno=saved;if(n<0)goto fail;
 int text=nova_desktop_text_prefix(out->text,(unsigned)n,r->offset+(unsigned)n==out->total);
 if(text<0||(!text&&n)){errno=ENOTSUP;goto fail;}
 out->length=(unsigned)text;memset(out->text+out->length,0,sizeof out->text-out->length);return 0;
fail:out->status=errno?errno:EIO;out->offset=0;out->length=0;memset(out->text,0,sizeof out->text);return -1;
}
/* One atomic no-replace rename. No copy fallback, recursive deletion, shell
 * command, overwrite or permanent-empty action. Original name is encoded in
 * the private token; even a crash needs no separate metadata transaction. */
static int nova_desktop_trash_change(int home,const struct nova_desktop_file_request*r,struct nova_desktop_file_reply*out){
 memset(out,0,sizeof *out);out->action=r->action;
 if(!nova_desktop_file_request_valid(r)||(r->action!=NOVA_DESKTOP_TRASH_MOVE&&r->action!=NOVA_DESKTOP_TRASH_RESTORE)){errno=EINVAL;goto fail;}
 int d=nova_desktop_dir(home);if(d<0)goto fail;
 int trash=nova_desktop_trash_dir(home,d);if(trash<0){int e=errno;close(d);errno=e;goto fail;}
 const char *original=r->action==NOVA_DESKTOP_TRASH_MOVE?r->name:r->name+19;strcpy(out->name,original);
 struct stat s;int source=r->action==NOVA_DESKTOP_TRASH_MOVE?d:trash;
 if(fstatat(source,r->name,&s,AT_SYMLINK_NOFOLLOW)){int e=errno;close(trash);close(d);errno=e;goto fail;}
 if(!nova_desktop_owned(&s)){close(trash);close(d);errno=EPERM;goto fail;}
 if(fsync(home)||fsync(trash)){int e=errno;close(trash);close(d);errno=e;goto fail;}
 int rc=-1;
 if(r->action==NOVA_DESKTOP_TRASH_MOVE){
  for(unsigned i=0;i<8;i++){unsigned long long nonce;ssize_t n=getrandom(&nonce,sizeof nonce,GRND_NONBLOCK);if(n!=sizeof nonce){errno=EAGAIN;break;}
   char token[96];snprintf(token,sizeof token,"t-%016llx-%.63s",nonce,r->name);
   rc=(int)syscall(SYS_renameat2,d,r->name,trash,token,1U);if(!rc||errno!=EEXIST)break;
  }
 }else rc=(int)syscall(SYS_renameat2,trash,r->name,d,original,1U);
 int saved=errno;
 if(!rc&&(fsync(d)||fsync(trash)||fsync(home))){rc=-1;saved=EIO;}
 close(trash);close(d);errno=saved;if(rc)goto fail;return 0;
fail:out->status=errno?errno:EIO;return -1;
}
static int nova_desktop_trash_page(int home,unsigned start,struct nova_desktop_trash_snapshot*out){
 memset(out,0,sizeof *out);if(start>256){errno=EINVAL;goto fail;}int d=nova_desktop_dir(home);if(d<0)goto fail;
 int fd=nova_desktop_trash_dir(home,d);int saved=errno;close(d);errno=saved;if(fd<0)goto fail;
 DIR*dir=fdopendir(fd);if(!dir){saved=errno;close(fd);errno=saved;goto fail;}
 /* Bounded scan and bounded stack; paging never drops or deletes other items. */
 struct nova_desktop_trash_entry entries[256];unsigned count=0,seen=0;struct dirent*e;
 while((e=readdir(dir))){if(++seen>1024){out->clipped=1;break;}if(!nova_desktop_token_valid(e->d_name))continue;
  struct stat s;if(fstatat(dirfd(dir),e->d_name,&s,AT_SYMLINK_NOFOLLOW)||!nova_desktop_owned(&s))continue;
  if(count==256){out->clipped=1;break;}
  struct nova_desktop_trash_entry*v=&entries[count++];memset(v,0,sizeof *v);v->kind=S_ISDIR(s.st_mode)?2:1;strcpy(v->name,e->d_name+19);strcpy(v->token,e->d_name);
 }
 closedir(dir);
 for(unsigned i=0;i<count;i++)for(unsigned j=i+1;j<count;j++)if(strcmp(entries[i].token,entries[j].token)>0){struct nova_desktop_trash_entry t=entries[i];entries[i]=entries[j];entries[j]=t;}
 if(start>count){errno=EINVAL;goto fail;}out->start=start;out->total=count;
 out->count=count-start;if(out->count>NOVA_DESKTOP_TRASH_LIMIT)out->count=NOVA_DESKTOP_TRASH_LIMIT;
 memcpy(out->entries,entries+start,out->count*sizeof entries[0]);return 0;
fail:out->status=errno?errno:EIO;return -1;
}
static inline int nova_desktop_trash_snapshot(int home,struct nova_desktop_trash_snapshot*out){return nova_desktop_trash_page(home,0,out);}
#endif
