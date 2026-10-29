#define _GNU_SOURCE
#include "nova_repository.h"
#include <openssl/evp.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#ifndef NR_CURL
#define NR_CURL "/usr/bin/curl"
#endif
static int number(const char*s,uint64_t*v){if(!*s||(*s=='0'&&s[1]))return 0;uint64_t n=0;for(;*s;s++){if(*s<'0'||*s>'9'||n>(UINT64_MAX-(unsigned)(*s-'0'))/10)return 0;n=n*10+(unsigned)(*s-'0');}*v=n;return 1;}
static int word(const char*s,size_t max){size_t n=strlen(s);if(!n||n>max)return 0;for(size_t i=0;i<n;i++)if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='0'&&s[i]<='9')||s[i]=='-'||s[i]=='.'))return 0;return s[0]!='.';}
static int split(char*line,char**fields,unsigned count){unsigned i=0;fields[i++]=line;for(char*p=line;*p;p++)if(*p=='\t'){if(i==count)return 0;*p=0;fields[i++]=p+1;}return i==count;}
static int url_parts(const char*url,char host[254]){
 if(strncmp(url,"https://",8))return 0;const char*p=strchr(url+8,'/');if(!p||p==url+8||(size_t)(p-url-8)>253)return 0;
 size_t n=(size_t)(p-url-8);memcpy(host,url+8,n);host[n]=0;
 for(size_t i=0;i<n;i++)if(!((host[i]>='a'&&host[i]<='z')||(host[i]>='0'&&host[i]<='9')||host[i]=='-'||host[i]=='.'))return 0;
 if(host[0]=='.'||host[n-1]=='.'||strstr(host,".."))return 0;
 for(p=url;*p;p++)if((unsigned char)*p<=32||*p=='\\'||*p=='#'||(unsigned char)*p>=127)return 0;return 1;
}
int nr_verify(const unsigned char*wire,size_t size,const unsigned char key[32],const char*arch,uint64_t floor,uint64_t now,struct nr_index*out){
 if(out)memset(out,0,sizeof *out);
 if(!wire||!key||!out||!arch||size<=64||size>NR_MAX_INDEX+64||(!strcmp(arch,"x86_64")?0:strcmp(arch,"arm64")))return EINVAL;
 memset(out,0,sizeof *out);EVP_PKEY*p=EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,NULL,key,32);EVP_MD_CTX*m=EVP_MD_CTX_new();int valid=p&&m&&EVP_DigestVerifyInit(m,NULL,NULL,NULL,p)==1&&EVP_DigestVerify(m,wire,64,wire+64,size-64)==1;EVP_MD_CTX_free(m);EVP_PKEY_free(p);if(!valid)return EBADMSG;
 const unsigned char*body=wire+64;size_t len=size-64;if(body[len-1]!='\n')return EPROTO;
 for(size_t i=0;i<len;i++)if((body[i]<32&&body[i]!='\n'&&body[i]!='\t')||body[i]>=127)return EPROTO;
 char*copy=malloc(len+1);if(!copy)return ENOMEM;memcpy(copy,body,len);copy[len]=0;int error=EPROTO;char*line=copy,*next=strchr(line,'\n');if(!next)goto done;*next++=0;char*f[6];
 if(!split(line,f,3)||strcmp(f[0],"NOVA-REPO-1")||!number(f[1],&out->serial)||!number(f[2],&out->expires))goto done;
 if(!out->serial||out->serial<floor){error=ESTALE;goto done;}if(out->expires<=now||out->expires-now>366ULL*86400){error=ETIME;goto done;}
 while(*next){line=next;next=strchr(line,'\n');if(!next)goto done;*next++=0;if(out->count==NR_MAX_ENTRIES)goto done;
  if(!split(line,f,6))goto done;struct nr_entry*e=&out->entries[out->count];char host[254];
  if(!word(f[0],48)||!word(f[1],32)||strcmp(f[2],arch)||strlen(f[4])!=64||strlen(f[5])>512||!url_parts(f[5],host)||!number(f[3],&e->bytes)||!e->bytes||e->bytes>NR_MAX_PAYLOAD)goto done;
  for(unsigned j=0;j<64;j++)if(!((f[4][j]>='0'&&f[4][j]<='9')||(f[4][j]>='a'&&f[4][j]<='f')))goto done;
  for(unsigned j=0;j<out->count;j++)if(!strcmp(out->entries[j].id,f[0]))goto done;
  strcpy(e->id,f[0]);strcpy(e->version,f[1]);strcpy(e->arch,f[2]);strcpy(e->sha256,f[4]);strcpy(e->url,f[5]);out->count++;
 }
 error=out->count?0:EPROTO;
 done:free(copy);if(error)memset(out,0,sizeof *out);return error;
}
static int public_ip(uint32_t v){unsigned a=v>>24,b=v>>16&255,c=v>>8&255;
#ifdef NR_TEST_LOOPBACK
 if(a==127)return 1;
#endif
 return !(a==0||a==10||a==127||a>=224||(a==100&&b>=64&&b<=127)||(a==169&&b==254)||(a==172&&b>=16&&b<=31)||(a==192&&(b==168||b==0||b==2))||(a==198&&(b==18||b==19||(b==51&&c==100)))||(a==203&&b==0&&c==113));}
int nr_https_fetch(const struct nr_entry*e,const char*ca,int output){
 char host[254];if(!e||strnlen(e->url,sizeof e->url)==sizeof e->url||strnlen(e->sha256,sizeof e->sha256)!=64||!ca||!e->bytes||e->bytes>NR_MAX_PAYLOAD||!url_parts(e->url,host))return EINVAL;struct stat st;
 if(fstat(output,&st)||!S_ISREG(st.st_mode)||st.st_size||st.st_uid!=getuid()||st.st_nlink>1)return EPERM;
 if(lseek(output,0,SEEK_SET)<0)return EIO;
 struct addrinfo hint={.ai_family=AF_INET,.ai_socktype=SOCK_STREAM},*all=NULL;if(getaddrinfo(host,"443",&hint,&all))return EHOSTUNREACH;
 char address[INET_ADDRSTRLEN]={0};for(struct addrinfo*p=all;p;p=p->ai_next){struct sockaddr_in*ip=(void*)p->ai_addr;if(p->ai_family==AF_INET&&public_ip(ntohl(ip->sin_addr.s_addr))&&inet_ntop(AF_INET,&ip->sin_addr,address,sizeof address))break;}freeaddrinfo(all);if(!*address)return EACCES;
 char resolve[320],limit[32];snprintf(resolve,sizeof resolve,"%s:443:%s",host,address);snprintf(limit,sizeof limit,"%llu",(unsigned long long)e->bytes);
 pid_t caller=getpid();pid_t child=fork();if(child<0)return errno;if(!child){if(prctl(PR_SET_PDEATHSIG,SIGTERM)||getppid()!=caller)_exit(126);if(dup2(output,3)<0||fcntl(3,F_SETFD,0)<0||close_range(4,~0U,0))_exit(126);int null=open("/dev/null",O_RDWR);if(null<0)_exit(126);if(null<=3){int other=fcntl(null,F_DUPFD_CLOEXEC,4);if(other<0)_exit(126);null=other;}for(int i=0;i<3;i++)if(dup2(i==1?3:null,i)<0||fcntl(i,F_SETFD,0)<0)_exit(126);if(null>3)close(null);
  struct rlimit bytes={e->bytes,e->bytes},core={0,0},cpu={15,15};if(setrlimit(RLIMIT_FSIZE,&bytes)||setrlimit(RLIMIT_CORE,&core)||setrlimit(RLIMIT_CPU,&cpu)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0))_exit(126);
  char*args[]={NR_CURL,"--disable","--globoff","--tlsv1.2","--proto","=https","--proto-redir","=https","--noproxy","*","--proxy","","--resolve",resolve,"--max-redirs","0","--fail","--silent","--show-error","--connect-timeout","5","--max-time","30","--max-filesize",limit,"--cacert",(char*)ca,"--output","-",(char*)e->url,NULL};char*env[]={"PATH=/usr/bin:/bin","LANG=C",NULL};execve(args[0],args,env);_exit(127);}
 int status=0;pid_t got;do got=waitpid(child,&status,0);while(got<0&&errno==EINTR);if(got!=child||!WIFEXITED(status)||WEXITSTATUS(status)){if(ftruncate(output,0))return EIO;return EIO;}
 if(fstat(output,&st)||(uint64_t)st.st_size!=e->bytes||lseek(output,0,SEEK_SET)<0){if(ftruncate(output,0))return EIO;return EBADMSG;}
 EVP_MD_CTX*m=EVP_MD_CTX_new();unsigned char b[16384],digest[32]={0};unsigned n=0;int ok=m&&EVP_DigestInit_ex(m,EVP_sha256(),NULL)==1;ssize_t count=0;while(ok&&(count=read(output,b,sizeof b))>0)ok=EVP_DigestUpdate(m,b,(size_t)count)==1;
 if(ok&&(count<0||EVP_DigestFinal_ex(m,digest,&n)!=1||n!=32))ok=0;EVP_MD_CTX_free(m);char hex[65];for(unsigned i=0;i<32;i++)sprintf(hex+i*2,"%02x",digest[i]);
 if(!ok||strcmp(hex,e->sha256)){if(ftruncate(output,0))return EIO;return EBADMSG;}return lseek(output,0,SEEK_SET)<0?EIO:0;
}

/* Caller supplies a private noexec cache directory and a root-pinned trust key.
 * This protects against network rollback, not a malicious owner deleting cache. */
static int owned_file(int fd){struct stat s;return fstat(fd,&s)||!S_ISREG(s.st_mode)||s.st_uid!=getuid()||s.st_nlink!=1||(s.st_mode&0777)!=0600;}
int nr_publish_index(int dir,const unsigned char*wire,size_t size,const unsigned char key[32],const char*arch,uint64_t floor,uint64_t now){
 struct stat ds;if(fstat(dir,&ds)||!S_ISDIR(ds.st_mode)||ds.st_uid!=getuid()||(ds.st_mode&0777)!=0700)return EPERM;
 int lock=openat(dir,".repository-lock",O_RDWR|O_CREAT|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC,0600);if(lock<0)return errno;if(owned_file(lock)){close(lock);return EPERM;}
 struct flock fl={.l_type=F_WRLCK,.l_whence=SEEK_SET};if(fcntl(lock,F_SETLKW,&fl)){int e=errno;close(lock);return e;}
 int rc=0,old=openat(dir,"repository.nri",O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);unsigned char*prior=NULL;struct nr_index*parsed=calloc(1,sizeof *parsed);if(!parsed){close(lock);return ENOMEM;}
 if(old>=0){struct stat st;if(owned_file(old)||fstat(old,&st)||st.st_size<=64||st.st_size>NR_MAX_INDEX+64){rc=EPERM;goto done;}prior=malloc((size_t)st.st_size);if(!prior){rc=ENOMEM;goto done;}size_t used=0;while(used<(size_t)st.st_size){ssize_t n=read(old,prior+used,(size_t)st.st_size-used);if(n<0&&errno==EINTR)continue;if(n<=0){rc=EIO;goto done;}used+=(size_t)n;}
  /* Validate even an expired cached index before using its signed serial. */
  char head[96];const unsigned char*end=memchr(prior+64,'\n',used-64);if(!end||(size_t)(end-prior-64)>=sizeof head){rc=EPROTO;goto done;}size_t n=(size_t)(end-prior-64);memcpy(head,prior+64,n);head[n]=0;char*f[3];uint64_t expiry;if(!split(head,f,3)||!number(f[2],&expiry)||!expiry){rc=EPROTO;goto done;}
  rc=nr_verify(prior,used,key,arch,0,expiry-1,parsed);if(rc)goto done;if(parsed->serial>floor)floor=parsed->serial;
 }else if(errno!=ENOENT){rc=errno;goto done;}
 rc=nr_verify(wire,size,key,arch,floor,now,parsed);if(rc)goto done;
 if(prior&&parsed->serial==floor){struct stat st;if(fstat(old,&st)){rc=EIO;goto done;}if((size_t)st.st_size!=size||memcmp(prior,wire,size)){rc=ESTALE;goto done;}goto done;}
 unsigned char random[8];if(getentropy(random,sizeof random)){rc=errno;goto done;}char name[48];snprintf(name,sizeof name,".index-%ld-%02x%02x%02x%02x",(long)getpid(),random[0],random[1],random[2],random[3]);int stage=openat(dir,name,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(stage<0){rc=errno;goto done;}
 size_t used=0;while(used<size){ssize_t n=write(stage,wire+used,size-used);if(n<0&&errno==EINTR)continue;if(n<=0){rc=EIO;break;}used+=(size_t)n;}
 if(!rc&&fsync(stage))rc=errno;if(close(stage)&&!rc)rc=errno;
 if(!rc&&renameat(dir,name,dir,"repository.nri"))rc=errno;if(rc){int saved=rc;if(unlinkat(dir,name,0)&&errno!=ENOENT)saved=EIO;rc=saved;}else if(fsync(dir))rc=errno;
 done:if(old>=0)close(old);free(prior);free(parsed);close(lock);return rc;
}
