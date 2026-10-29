#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
static int fd_count(void){int n=0;for(int i=0;i<1024;i++)if(fcntl(i,F_GETFD)>=0)n++;return n;}
static void ancillary_test(int source,unsigned int count,unsigned int size){
 int pair[2];assert(socketpair(AF_UNIX,SOCK_SEQPACKET,0,pair)==0);
 unsigned char data[sizeof(struct nova_surface_message)+1U]={0};int rights[10];for(int i=0;i<10;i++)rights[i]=source;
 char control[CMSG_SPACE(sizeof(rights))];struct iovec iov={data,size};struct msghdr msg={0};msg.msg_iov=&iov;msg.msg_iovlen=1;
 if(count){msg.msg_control=control;msg.msg_controllen=CMSG_SPACE(count*sizeof(int));struct cmsghdr*c=CMSG_FIRSTHDR(&msg);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(count*sizeof(int));nova_copy(CMSG_DATA(c),rights,count*sizeof(int));}
 assert(sendmsg(pair[0],&msg,0)==(long)size);int before=fd_count(),received=-1;struct nova_surface_message packet;
 long n=recv_surface_packet(pair[1],&packet,&received);
 if(count<=1&&size==sizeof(packet)){
  assert(n==(long)sizeof(packet));if(count){assert(received>=0&&(fcntl(received,F_GETFD)&FD_CLOEXEC));close(received);}else assert(received==-1);
 }else assert(n<0&&received==-1);
 assert(fd_count()==before);close(pair[0]);close(pair[1]);
}
int main(void){
 long fd=nova_memfd_create("untrusted",NOVA_MFD_CLOEXEC);assert(fd>=0);
 ancillary_test((int)fd,0,sizeof(struct nova_surface_message));ancillary_test((int)fd,1,sizeof(struct nova_surface_message));
 ancillary_test((int)fd,2,sizeof(struct nova_surface_message));ancillary_test((int)fd,10,sizeof(struct nova_surface_message));
 ancillary_test((int)fd,1,0);ancillary_test((int)fd,2,sizeof(struct nova_surface_message)+1U);
 assert(nova_ftruncate((int)fd,1600000)==0);unsigned int first=0x00123456;
 assert(nova_write((int)fd,&first,sizeof(first))==sizeof(first));
 struct nova_surface_message m;nova_zero(&m,sizeof(m));m.client_id=3;m.buffer_id=1;
 m.width=800;m.height=500;m.stride=3200;m.buffer_bytes=1600000;m.pixel_format=NOVA_PIXEL_FORMAT_XRGB8888;
 assert(register_client_buffer(99,&m,(int)fd,1600,900));
 struct nova_client_buffer *b=find_client_buffer(3,1);assert(b&&!b->map);
 assert(r17_refresh_client_buffer(b));assert(((unsigned int*)b->map)[0]==first);
 void *previous=b->map;assert(nova_ftruncate((int)fd,0)==0);
 assert(!r17_refresh_client_buffer(b));assert(b->map==previous&&((unsigned int*)b->map)[0]==first);
 assert(nova_ftruncate((int)fd,1600000)==0);
 for(int i=0;i<50;i++){assert(r17_refresh_client_buffer(b));assert(b->map);}
 destroy_client_buffer_entry(b);assert(!b->used);
 puts("R17_COMPOSITOR_ANCILLARY_MULTI_FD_ZERO_BODY_TRUNCATION_CLOEXEC_LEAK_DENIAL=PASS");
 puts("R17_REAL_SHRINKABLE_MEMFD_TRUNCATE_NO_SIGBUS_ATOMIC_SHADOW_AND_CLEANUP=PASS");return 0;
}
