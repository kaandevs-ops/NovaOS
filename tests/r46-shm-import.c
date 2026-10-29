#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include "nova_shm_import.h"
struct race {int fd;atomic_int stop;};
static void*shrink(void*p){struct race*r=p;while(!atomic_load(&r->stop)){assert(!ftruncate(r->fd,0));assert(!ftruncate(r->fd,4096));}return NULL;}
int main(void){int fd=memfd_create("nova-r46-test",MFD_CLOEXEC|MFD_ALLOW_SEALING);assert(fd>=0);assert(!ftruncate(fd,4096));uint32_t pixels[]={0xff123456,0x00112233,0x80800000,0};assert(pwrite(fd,pixels,sizeof pixels,16)==sizeof pixels);
 int ipc[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET,0,ipc));char marker='B';struct iovec io={&marker,1};union{struct cmsghdr a;unsigned char bytes[CMSG_SPACE(sizeof(int))];}control={0};struct msghdr msg={.msg_iov=&io,.msg_iovlen=1,.msg_control=control.bytes,.msg_controllen=sizeof control};struct cmsghdr*c=CMSG_FIRSTHDR(&msg);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(c),&fd,sizeof fd);assert(sendmsg(ipc[0],&msg,0)==1);memset(&control,0,sizeof control);assert(recvmsg(ipc[1],&msg,MSG_CMSG_CLOEXEC)==1);c=CMSG_FIRSTHDR(&msg);assert(c&&c->cmsg_level==SOL_SOCKET&&c->cmsg_type==SCM_RIGHTS);int received;memcpy(&received,CMSG_DATA(c),sizeof received);assert(received>=0&&received!=fd&&(fcntl(received,F_GETFD)&FD_CLOEXEC));close(ipc[0]);close(ipc[1]);
 struct nova_shm_spec s={2,2,8,NOVA_SHM_XRGB8888,16,4096};unsigned char target[48];memset(target,0x5a,sizeof target);assert(!nova_shm_import(fd,getuid(),&s,0,target+16,16));assert(!nova_shm_import(received,getuid(),&s,0,target+16,16));close(received);uint32_t result[4];memcpy(result,target+16,16);assert(result[0]==0x123456&&result[1]==0x112233);assert(lseek(fd,0,SEEK_CUR)==0);
 for(int i=0;i<16;i++)assert(target[i]==0x5a&&target[32+i]==0x5a);
 s.format=NOVA_SHM_ARGB8888;assert(!nova_shm_import(fd,getuid(),&s,0xffffff,target+16,16));memcpy(result,target+16,16);assert(result[0]==0x123456&&result[1]==0xffffff&&result[2]==0xff7f7f&&result[3]==0xffffff);
 assert(nova_shm_import(fd,getuid()+1,&s,0,target+16,16)<0);assert(nova_shm_import(fd,getuid(),&s,0,target+16,15)<0);
 struct nova_shm_spec bad=s;bad.offset=UINT64_MAX;assert(nova_shm_import(fd,getuid(),&bad,0,target+16,16)<0);bad=s;bad.stride=UINT32_MAX;assert(nova_shm_import(fd,getuid(),&bad,0,target+16,16)<0);bad=s;bad.width=1921;assert(nova_shm_import(fd,getuid(),&bad,0,target+16,16)<0);bad=s;bad.format=2;assert(nova_shm_import(fd,getuid(),&bad,0,target+16,16)<0);
 int pipefd[2];assert(!pipe(pipefd));assert(nova_shm_import(pipefd[0],getuid(),&s,0,target+16,16)<0);close(pipefd[0]);close(pipefd[1]);int pair[2];assert(!socketpair(AF_UNIX,SOCK_STREAM,0,pair));assert(nova_shm_import(pair[0],getuid(),&s,0,target+16,16)<0);close(pair[0]);close(pair[1]);
 assert(!ftruncate(fd,8));assert(nova_shm_import(fd,getuid(),&s,0,target+16,16)<0);assert(!ftruncate(fd,4096));
 struct race race={.fd=fd};pthread_t th;assert(!pthread_create(&th,NULL,shrink,&race));for(unsigned i=0;i<10000;i++){(void)nova_shm_import(fd,getuid(),&s,0,target+16,16);for(int j=0;j<16;j++)assert(target[j]==0x5a&&target[32+j]==0x5a);}atomic_store(&race.stop,1);assert(!pthread_join(th,NULL));assert(fcntl(fd,F_GETFD)>=0);close(fd);
 puts("SHM_IMPORT_FORMAT_ALPHA_BOUNDS_OWNER_FD_TYPES_OFFSET_NO_SIGBUS_10000_TRUNCATE_RACE_PASS");return 0;}
