#include "nova_files_core.h"
/* Nova Files R14: freestanding, deterministic and transactional directory model.
 * No traversal by string concatenation: directory authority is a bounded FD stack.
 * This library does not itself grant filesystem access. */
#define NOVA_DIRENT_NAME_OFFSET 19U
#define NOVA_DIRENT_SCRATCH 4096U
#define NOVA_DIRENT_MIN_SIZE 21U
static void memory_zero(void *p,unsigned long n){unsigned char *b=(unsigned char *)p;for(unsigned long i=0;i<n;i++)b[i]=0;}
static unsigned int bounded_length(const char*s,unsigned int lim){unsigned int i=0;if(!s)return lim;while(i<lim&&s[i])i++;return i;}
static int name_valid(const char*s){unsigned int n=bounded_length(s,NOVA_FILES_NAME_BYTES);
 if(n==0U||n>=NOVA_FILES_NAME_BYTES)return 0;
 if((n==1U&&s[0]=='.')||(n==2U&&s[0]=='.'&&s[1]=='.'))return 0;
 for(unsigned int i=0;i<n;i++){unsigned char c=(unsigned char)s[i];if(c=='/'||c=='\\'||c<32U||c==127U)return 0;}
 return 1;
}
static unsigned int little16(const unsigned char*b){return (unsigned int)b[0]|((unsigned int)b[1]<<8);}
static int name_compare(const char*a,const char*b){unsigned int i=0;for(;;i++){
 unsigned char ca=(unsigned char)a[i],cb=(unsigned char)b[i];
 if(ca!=cb)return ca<cb?-1:1;
 if(!ca)return 0;
 }}
/* Ordering is predictable on ext4, tmpfs and any filesystem, independent of getdents64 order. */
static int entry_compare(const struct nova_files_entry*a,const struct nova_files_entry*b){
 unsigned int aa=a->type==NOVA_FILES_DTYPE_DIR?0U:1U;
 unsigned int bb=b->type==NOVA_FILES_DTYPE_DIR?0U:1U;
 if(aa!=bb)return aa<bb?-1:1;
 return name_compare(a->name,b->name);
}
int nova_files_init(struct nova_files_model*m,int fd,const struct nova_files_ops*ops,void*ctx){
 if(!m||fd<0||!ops||!ops->seek_start||!ops->read_entries||!ops->open_child_dir||!ops->close_fd)return NOVA_FILES_BAD;
 memory_zero(m,sizeof(*m));m->ops=*ops;m->ctx=ctx;
 for(unsigned int i=0;i<=NOVA_FILES_MAX_DEPTH;i++)m->directory_fds[i]=-1;
 m->directory_fds[0]=fd;m->path[0]='/';m->path[1]=0;m->path_marks[0]=1U;return NOVA_FILES_OK;
}
static unsigned char query_fold(unsigned char c){return c>='A'&&c<='Z'?(unsigned char)(c+32):c;}
static int query_matches(const char*name,const char*query){
 if(!query[0])return 1;
 for(unsigned int start=0;name[start];start++){
  unsigned int i=0;while(query[i]&&name[start+i]&&query_fold((unsigned char)name[start+i])==query_fold((unsigned char)query[i]))i++;
  if(!query[i])return 1;
 }return 0;
}
/* Keep only the first/last 48 ordered eligible names, regardless of kernel enumeration
 * order. No heap or unbounded directory cache; all FD reads retain the old budget. */
static void select_entry(struct nova_files_entry*staged,unsigned int*count,
                         const struct nova_files_entry*entry,int backwards){
 if(*count==NOVA_FILES_MAX_ENTRIES){
  if(!backwards){if(entry_compare(entry,&staged[*count-1U])>=0)return;(*count)--;}
  else{if(entry_compare(entry,&staged[0])<=0)return;for(unsigned int i=1;i<*count;i++)staged[i-1U]=staged[i];(*count)--;}
 }
 unsigned int at=*count;
 while(at&&entry_compare(entry,&staged[at-1U])<0){staged[at]=staged[at-1U];at--;}
 staged[at]=*entry;(*count)++;
}
static int scan_page(struct nova_files_model*m,const char*query,
                     const struct nova_files_entry*anchor,int backwards,int require_page){
 if(!m||!query||!m->ops.seek_start||!m->ops.read_entries||m->depth>NOVA_FILES_MAX_DEPTH||m->directory_fds[m->depth]<0)return NOVA_FILES_BAD;
 unsigned int qlen=bounded_length(query,NOVA_FILES_QUERY_BYTES);
 if(qlen>=NOVA_FILES_QUERY_BYTES)return NOVA_FILES_BAD;
 char query_copy[NOVA_FILES_QUERY_BYTES];for(unsigned int i=0;i<=qlen;i++)query_copy[i]=query[i];query=query_copy;
 if(m->ops.seek_start(m->directory_fds[m->depth],m->ctx)<0)return NOVA_FILES_IO;
 struct nova_files_entry staged[NOVA_FILES_MAX_ENTRIES];
 unsigned int count=0,scanned=0,eligible=0,excluded=0;
 memory_zero(staged,sizeof(staged));unsigned char block[NOVA_DIRENT_SCRATCH];
 for(unsigned int batches=0;batches<256U;batches++){
  long n=m->ops.read_entries(m->directory_fds[m->depth],block,sizeof(block),m->ctx);
  if(n<0||n>(long)sizeof(block))return NOVA_FILES_IO;
  if(n==0){
   if(require_page&&!count)return NOVA_FILES_FULL;
   memory_zero(m->entries,sizeof(m->entries));for(unsigned int i=0;i<count;i++)m->entries[i]=staged[i];
   m->count=count;m->scanned=scanned;m->truncated=scanned>count;
   m->has_previous=backwards?eligible>NOVA_FILES_MAX_ENTRIES:excluded;
   m->has_next=backwards?excluded:eligible>NOVA_FILES_MAX_ENTRIES;
   memory_zero(m->listing_query,sizeof(m->listing_query));for(unsigned int i=0;i<=qlen;i++)m->listing_query[i]=query[i];
   return NOVA_FILES_OK;
  }
  unsigned long pos=0;
  while(pos<(unsigned long)n){
   if((unsigned long)n-pos<NOVA_DIRENT_MIN_SIZE)return NOVA_FILES_IO;
   unsigned int len=little16(block+pos+16U);
   if(len<NOVA_DIRENT_MIN_SIZE||len>(unsigned long)n-pos)return NOVA_FILES_IO;
   const char*s=(const char*)block+pos+NOVA_DIRENT_NAME_OFFSET;
   unsigned int capacity=len-NOVA_DIRENT_NAME_OFFSET,namelen=bounded_length(s,capacity);
   if(namelen==capacity)return NOVA_FILES_IO;
   if(name_valid(s)){
    scanned++;
    if(query_matches(s,query)){
     struct nova_files_entry entry;memory_zero(&entry,sizeof(entry));for(unsigned int i=0;i<=namelen;i++)entry.name[i]=s[i];entry.type=block[pos+18U];
     int order=anchor?entry_compare(&entry,anchor):0;
     if(anchor&&(backwards?order>=0:order<=0))excluded=1;
     else{eligible++;select_entry(staged,&count,&entry,backwards);}
    }
   }
   pos+=len;
  }
 }
 return NOVA_FILES_FULL;/* Repeating/oversized enumeration preserves the prior snapshot. */
}
int nova_files_list(struct nova_files_model*m){return scan_page(m,"",0,0,0);}
int nova_files_query(struct nova_files_model*m,const char*query){return scan_page(m,query,0,0,0);}
int nova_files_page(struct nova_files_model*m,int forward){
 if(!m||!m->count||m->count>NOVA_FILES_MAX_ENTRIES)return NOVA_FILES_BAD;
 if(forward!=0&&forward!=1)return NOVA_FILES_BAD;
 if(forward?!m->has_next:!m->has_previous)return NOVA_FILES_FULL;
 struct nova_files_entry anchor=m->entries[forward?m->count-1U:0U];
 /* query is copied before scan_page commits into the model. */
 char query[NOVA_FILES_QUERY_BYTES];for(unsigned int i=0;i<sizeof(query);i++)query[i]=m->listing_query[i];
 return scan_page(m,query,&anchor,!forward,1);
}int nova_files_enter(struct nova_files_model*m,const char*name){
 if(!m||!name_valid(name)||m->depth>NOVA_FILES_MAX_DEPTH)return NOVA_FILES_ESCAPE;
 if(m->depth>=NOVA_FILES_MAX_DEPTH)return NOVA_FILES_FULL;
 unsigned int n=bounded_length(name,NOVA_FILES_NAME_BYTES);
 unsigned int old=bounded_length(m->path,NOVA_FILES_PATH_BYTES);
 if(old>=NOVA_FILES_PATH_BYTES||old+n+2U>=NOVA_FILES_PATH_BYTES)return NOVA_FILES_FULL;
 int fd=(int)m->ops.open_child_dir(m->directory_fds[m->depth],name,m->ctx);
 if(fd<0)return NOVA_FILES_IO;
 unsigned int end=old;
 if(end>1U)m->path[end++]='/';
 for(unsigned int i=0;i<n;i++)m->path[end++]=name[i];
 m->path[end]=0;
 m->depth++;m->directory_fds[m->depth]=fd;m->path_marks[m->depth]=end;
 int rc=nova_files_list(m);
 if(rc!=NOVA_FILES_OK){
  m->ops.close_fd(fd,m->ctx);m->directory_fds[m->depth]=-1;
  m->depth--;m->path[old]=0;
  /* Previous snapshot was preserved by transactional nova_files_list. */
  return rc;
 }
 return NOVA_FILES_OK;
}
int nova_files_parent(struct nova_files_model*m){
 if(!m||m->depth==0U||m->depth>NOVA_FILES_MAX_DEPTH)return NOVA_FILES_ESCAPE;
 int fd=m->directory_fds[m->depth];
 m->ops.close_fd(fd,m->ctx);m->directory_fds[m->depth]=-1;
 m->depth--;unsigned int n=m->path_marks[m->depth];
 if(n>=NOVA_FILES_PATH_BYTES)return NOVA_FILES_BAD;
 m->path[n]=0;
 int rc=nova_files_list(m);
 if(rc!=NOVA_FILES_OK){m->count=0;m->scanned=0;m->truncated=0;m->has_previous=0;m->has_next=0;}
 return rc;
}
void nova_files_close(struct nova_files_model*m){
 if(!m||!m->ops.close_fd)return;
 for(unsigned int i=0;i<=NOVA_FILES_MAX_DEPTH;i++){
  if(m->directory_fds[i]>=0){m->ops.close_fd(m->directory_fds[i],m->ctx);m->directory_fds[i]=-1;}
 }
 m->count=0;m->depth=0;m->scanned=0;m->truncated=0;m->has_previous=0;m->has_next=0;
}

/* Stage new descriptors and listing before replacing the active view. The root FD
 * remains owned by the caller's model on every path, including failures. */
int nova_files_navigate(struct nova_files_model*m,const char*path){
 if(!m||!path||path[0]!='/'||m->depth>NOVA_FILES_MAX_DEPTH)return NOVA_FILES_ESCAPE;
 unsigned int len=bounded_length(path,NOVA_FILES_PATH_BYTES);if(!len||len>=NOVA_FILES_PATH_BYTES)return NOVA_FILES_FULL;
 unsigned int pos=1,components=0;char name[NOVA_FILES_NAME_BYTES];
 while(pos<len){unsigned int n=0;while(pos<len&&path[pos]!='/'){if(n+1>=sizeof(name))return NOVA_FILES_FULL;name[n++]=path[pos++];}name[n]=0;
  if(!name_valid(name)||++components>NOVA_FILES_MAX_DEPTH)return NOVA_FILES_ESCAPE;
  if(pos<len&&++pos==len)return NOVA_FILES_ESCAPE;
 }
 struct nova_files_model staged;int rc=nova_files_init(&staged,m->directory_fds[0],&m->ops,m->ctx);if(rc)return rc;
 rc=nova_files_list(&staged);pos=1;
 while(!rc&&pos<len){unsigned int n=0;while(pos<len&&path[pos]!='/')name[n++]=path[pos++];name[n]=0;if(pos<len)pos++;rc=nova_files_enter(&staged,name);}
 if(rc){for(unsigned int i=1;i<=staged.depth;i++)staged.ops.close_fd(staged.directory_fds[i],staged.ctx);return rc;}
 for(unsigned int i=1;i<=m->depth;i++)m->ops.close_fd(m->directory_fds[i],m->ctx);
 *m=staged;return NOVA_FILES_OK;
}
