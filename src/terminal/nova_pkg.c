#define _GNU_SOURCE
/* Local fixed-catalog installer. All operations run as the terminal user.
 * Executables remain on noexec home; only verified, sealed bytes may execute. */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "../runtime/nova_sha256.h"
#include "../runtime/nova_local_catalog.h"
#define MAX_BYTES (8U*1024U*1024U)
#define EXEC_MEMFD_FLAGS 0x13U /* CLOEXEC | ALLOW_SEALING | EXEC */
static int fail(const char *why){fprintf(stderr,"nova-pkg: %s: %s\n",why,strerror(errno));return 1;}
static int check(int f,uid_t owner,int directory){struct stat s;if(fstat(f,&s))return -1;if((directory?!S_ISDIR(s.st_mode):!S_ISREG(s.st_mode))||s.st_uid!=owner||(directory&&(s.st_mode&0777)!=0700)){errno=EPERM;return -1;}return 0;}
static int store(void){
 int h=open("/home/nova",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(h<0)return -1;
 if(check(h,getuid(),1)){close(h);return -1;}
 if(mkdirat(h,".nova-apps",0700)&&errno!=EEXIST){close(h);return -1;}
 int d=openat(h,".nova-apps",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);close(h);
 if(d>=0&&check(d,getuid(),1)){close(d);return -1;}return d;
}
static int copy_verified(int src,int dst,const char *expected_sha256){
 struct stat st;if(fstat(src,&st)||!S_ISREG(st.st_mode)||st.st_size<=0||st.st_size>MAX_BYTES){errno=EINVAL;return -1;}
 struct nova_sha256_ctx hash;nova_sha256_init(&hash);unsigned char buf[16384],digest[32];char hex[65];size_t total=0;
 for(;;){ssize_t n=read(src,buf,sizeof buf);if(n<0&&errno==EINTR)continue;if(n<0)return -1;if(!n)break;
  total+=(size_t)n;if(total>MAX_BYTES){errno=EFBIG;return -1;}nova_sha256_update(&hash,buf,(size_t)n);
  if(dst>=0){size_t done=0;while(done<(size_t)n){ssize_t w=write(dst,buf+done,(size_t)n-done);if(w<0&&errno==EINTR)continue;if(w<=0)return -1;done+=(size_t)w;}}
 }
 nova_sha256_final(&hash,digest);nova_sha256_hex(digest,hex);
 if(total!=(size_t)st.st_size||strcmp(hex,expected_sha256)){errno=EBADMSG;return -1;}return 0;
}
static void json_text(const char*s){
 putchar('"');for(const unsigned char*p=(const unsigned char*)s;*p;p++){if(*p=='"'||*p=='\\'){putchar('\\');putchar(*p);}else if(*p<32)printf("\\u%04x",*p);else putchar(*p);}putchar('"');
}
static void metadata(const struct nova_catalog_entry*a){
 fputs("{\"id\":",stdout);json_text(a->id);fputs(",\"version\":",stdout);json_text(a->version);fputs(",\"title\":",stdout);json_text(a->title);fputs(",\"summary\":",stdout);json_text(a->summary);fputs(",\"category\":",stdout);json_text(a->category);fputs(",\"sha256\":",stdout);json_text(a->sha256);fputs(",\"interface\":\"terminal\",\"network_install\":false,\"source\":\"local-system-catalog\"}",stdout);
}
static int installed_state(int d,const struct nova_catalog_entry*a){
 int fd=openat(d,a->id,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
 if(fd<0)return errno==ENOENT?0:-1;
 int rc=check(fd,getuid(),0)||copy_verified(fd,-1,a->sha256);int saved=errno;close(fd);errno=saved;return rc?-1:1;
}
int main(int argc,char **argv){
 if(getuid()!=12004||geteuid()!=12004){errno=EPERM;return fail("terminal user required");}
 if((argc==2||argc==3)&&!strcmp(argv[1],"list")){
  int json=argc==3&&!strcmp(argv[2],"--json");if(argc==3&&!json)return 2;
  if(json)putchar('[');
  for(unsigned i=0;i<sizeof(nova_catalog)/sizeof(nova_catalog[0]);i++){if(json){if(i)putchar(',');metadata(&nova_catalog[i]);}else printf("%s %s - local verified catalog; install/run/remove; network unavailable\n",nova_catalog[i].id,nova_catalog[i].version);}
  if(json)puts("]");return 0;
 }
 if(argc==3&&!strcmp(argv[1],"search")){
  if(!argv[2][0]||strlen(argv[2])>80)return 2;
  for(const unsigned char*p=(unsigned char*)argv[2];*p;p++)if(*p<32||*p==127)return 2;
  unsigned count=0;
  for(unsigned i=0;i<sizeof(nova_catalog)/sizeof(nova_catalog[0]);i++){const struct nova_catalog_entry*a=&nova_catalog[i];if(strcasestr(a->id,argv[2])||strcasestr(a->title,argv[2])||strcasestr(a->summary,argv[2])||strcasestr(a->category,argv[2])){printf("%s %s - %s: %s\n",a->id,a->version,a->title,a->summary);count++;}}
  if(!count)puts("No matching package in the local catalog.");return 0;
 }
 if((argc==3||argc==4)&&!strcmp(argv[1],"info")){
  const struct nova_catalog_entry*a=find_package(argv[2]);if(!a)return 2;
  if(argc==4){if(strcmp(argv[3],"--json"))return 2;metadata(a);putchar('\n');}
  else printf("%s (%s)\n%s\nCategory: %s\nInterface: terminal\nSource: local verified catalog; no network installation\nInstall: nova-pkg install %s\nRun: nova-pkg run %s\nRemove: nova-pkg remove %s\n",a->title,a->version,a->summary,a->category,a->id,a->id,a->id);
  return 0;
 }
 if(argc==2&&!strcmp(argv[1],"installed")){
  int d=store();if(d<0)return fail("private package store");unsigned count=0;int bad=0;
  for(unsigned i=0;i<sizeof(nova_catalog)/sizeof(nova_catalog[0]);i++){int state=installed_state(d,&nova_catalog[i]);if(state<0){fprintf(stderr,"%s: INVALID installed package\n",nova_catalog[i].id);bad=1;}else if(state){printf("%s %s - installed, verified\n",nova_catalog[i].id,nova_catalog[i].version);count++;}}
  close(d);if(!count&&!bad)puts("No packages installed.");return bad;
 }
 if(argc==4&&!strcmp(argv[1],"status")&&!strcmp(argv[3],"--machine")){
  const struct nova_catalog_entry*a=find_package(argv[2]);if(!a)return 2;
  int d=store();if(d<0)return 1;int state=installed_state(d,a);close(d);
  return state==1?0:state==0?3:1; /* no stdout, no write lock needed for atomic inode reads */
 }
 if(argc<3||argc>64||!find_package(argv[2])){fputs("Usage: nova-pkg list [--json] | search TEXT | info ID [--json] | installed | install ID | upgrade ID | run ID [arguments] | remove ID | status ID\n",stderr);return 2;}
 const struct nova_catalog_entry *app=find_package(argv[2]);
 const char *op=argv[1];int imported=!strcmp(op,"install-fd");int input_fd=-1;
 if(imported){if(argc!=4)return 2;char*end;errno=0;long v=strtol(argv[3],&end,10);if(errno||!*argv[3]||*end||v<3||v>1024)return 2;input_fd=(int)v;}
 if(!imported&&strcmp(op,"run")&&strcmp(op,"install")&&strcmp(op,"upgrade")&&strcmp(op,"remove")&&strcmp(op,"status")){fputs("nova-pkg: unknown operation\n",stderr);return 2;}
 if(!imported&&strcmp(op,"run")&&argc!=3)return 2;
 int d=store();if(d<0)return fail("private package store");
 int lock=openat(d,".lock",O_RDWR|O_CREAT|O_NOFOLLOW|O_CLOEXEC,0600);if(lock<0||check(lock,getuid(),0))return fail("store lock");
 struct flock fl={.l_type=F_WRLCK,.l_whence=SEEK_SET};if(fcntl(lock,F_SETLKW,&fl))return fail("store busy");
 if(!strcmp(op,"status")){
  int f=openat(d,app->id,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
  if(f<0&&errno==ENOENT){printf("%s: not installed\n",app->id);return 0;}
  if(f<0||check(f,getuid(),0)||copy_verified(f,-1,app->sha256))return fail("installed package invalid");
  printf("%s: installed, verified; files persist with home, runtime remains sandboxed\n",app->id);return 0;
 }
 if(!strcmp(op,"remove")){
  if(unlinkat(d,app->id,0)&&errno!=ENOENT)return fail("remove");if(fsync(d))return fail("store sync");printf("%s: removed; user databases preserved\n",app->id);return 0;
 }
 if(imported||!strcmp(op,"install")||!strcmp(op,"upgrade")){
  int upgrade=!strcmp(op,"upgrade");
  struct stat st;if(!fstatat(d,app->id,&st,AT_SYMLINK_NOFOLLOW)){
   if(!upgrade){errno=EEXIST;return fail("already installed; use upgrade");}
   if(!S_ISREG(st.st_mode)||st.st_uid!=getuid()||st.st_nlink!=1){errno=EPERM;return fail("unsafe upgrade destination");}
  }else{if(errno!=ENOENT)return fail("destination");if(upgrade)return fail("upgrade requires installed package");}
  int src=imported?fcntl(input_fd,F_DUPFD_CLOEXEC,3):open(app->payload,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if(src<0)return fail("catalog payload");
  if((imported&&lseek(src,0,SEEK_SET)<0)||check(src,imported?getuid():0,0))return fail("untrusted catalog owner");
  char name[64];snprintf(name,sizeof name,".install-%ld",(long)getpid());
  /* The store lock serializes installs across terminal sessions. Recover an
   * interrupted staging inode only after type/owner/mode/link validation. */
  int dst=openat(d,name,O_WRONLY|O_CREAT|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC,0600);if(dst<0)return fail("staging file");
  struct stat partial;
  if(fstat(dst,&partial)||!S_ISREG(partial.st_mode)||partial.st_uid!=getuid()||partial.st_nlink!=1||(partial.st_mode&0777)!=0600){close(src);close(dst);errno=EPERM;return fail("unsafe staging file");}
  if(ftruncate(dst,0)){close(src);close(dst);return fail("staging recovery");}
  /* Keep unpublished staging recoverable (0600) until the atomic rename.
   * A crash after publication may leave 0600, which is safe: every run hashes
   * the bytes into a sealed image. Owner-read-only is convenience, not trust. */
  int rc=copy_verified(src,dst,app->sha256);if(!rc)rc=fsync(dst);int saved=rc?errno:0;close(src);
  if(!rc){rc=upgrade?renameat(d,name,d,app->id):(int)syscall(SYS_renameat2,d,name,d,app->id,1U);if(rc)saved=errno;}
  if(!rc){rc=fchmod(dst,0400);if(!rc)rc=fsync(dst);if(rc)saved=errno;}close(dst);
  if(rc){if(saved)errno=saved;int e=errno;unlinkat(d,name,0);errno=e;return fail("install rejected");}
  if(fsync(d))return fail("store sync");printf(upgrade?"%s: upgraded atomically from verified local catalog; user data preserved\n":"%s: installed from verified local catalog\n",app->id);return 0;
 }
 if(!strcmp(op,"run")){
  int src=openat(d,app->id,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);if(src<0)return fail("install the selected package first");
  if(check(src,getuid(),0))return fail("package owner/type");
  int image=(int)syscall(SYS_memfd_create,"nova-verified-app",EXEC_MEMFD_FLAGS);if(image<0)return fail("sealed executable buffer");
  if(copy_verified(src,image,app->sha256))return fail("package digest mismatch");close(src);
  if(fcntl(image,F_ADD_SEALS,F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK|F_SEAL_SEAL))return fail("seal executable");
  /* Recheck AFTER sealing: another same-UID process might have accessed the
   * unsealed memfd through procfs while it was being populated. */
  if(lseek(image,0,SEEK_SET)<0||copy_verified(image,-1,app->sha256))return fail("sealed image digest mismatch");
  char path[64];snprintf(path,sizeof path,"/proc/self/fd/%d",image);char *args[64];args[0]=(char *)app->command;
  for(int i=3;i<argc;i++)args[i-2]=argv[i];args[argc-2]=NULL;
  char *env[]={"HOME=/home/nova","PATH=/bin","TERM=dumb","LANG=C","USER=nova",NULL};
  close(lock);close(d);execve(path,args,env);return fail("execute verified package");
 }
 fputs("nova-pkg: unknown operation\n",stderr);return 2;
}
