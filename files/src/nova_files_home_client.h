#ifndef NOVA_FILES_HOME_CLIENT_H
#define NOVA_FILES_HOME_CLIENT_H
#include "nova_home_directory_protocol.h"
static struct {int shared,socket,details;unsigned sequence;int fds[NOVA_HOME_DIRECTORY_HANDLES];unsigned tokens[NOVA_HOME_DIRECTORY_HANDLES];} home_client={.socket=-1,.details=-1};
static void home_client_failed(void){if(home_client.socket>=0)nova_close(home_client.socket);home_client.socket=-1;}
static int home_socket_trusted(int fd){
 struct nova_ucred peer;unsigned bytes=sizeof peer;int type=0;unsigned len=sizeof type;
 return nova_getsockopt(fd,NOVA_SOL_SOCKET,3,&type,&len)>=0&&len==sizeof type&&type==NOVA_SOCK_SEQPACKET&&nova_getsockopt(fd,NOVA_SOL_SOCKET,NOVA_SO_PEERCRED,&peer,&bytes)>=0&&bytes==sizeof peer&&peer.uid==0&&peer.pid==(int)nova_getpid();
}
static unsigned home_receive_rights(struct nova_msghdr *m,unsigned char *control,int *fds,unsigned cap,int *valid){
 unsigned count=0;unsigned long pos=0;
 while(pos+sizeof(struct nova_cmsghdr)<=m->msg_controllen){
  struct nova_cmsghdr*c=(void*)(control+pos);if(c->cmsg_len<NOVA_CMSG_DATA_OFFSET||c->cmsg_len>m->msg_controllen-pos){*valid=0;break;}
  unsigned long bytes=c->cmsg_len-NOVA_CMSG_DATA_OFFSET;
  if(c->cmsg_level!=NOVA_SOL_SOCKET||c->cmsg_type!=NOVA_SCM_RIGHTS)*valid=0;
  else{
   if(bytes%sizeof(int))*valid=0;
   for(unsigned long n=0;n<bytes/sizeof(int);n++){int fd;nova_copy(&fd,control+pos+NOVA_CMSG_DATA_OFFSET+n*sizeof(int),sizeof fd);if(count<cap)fds[count++]=fd;else{nova_close(fd);*valid=0;}}
  }
  pos+=NOVA_CMSG_SPACE(bytes);
 }
 return count;
}
/* v1 one-directory grants remain supported for unprovisioned/legacy builds.
 * v2 adds a private worker channel, not broader app filesystem permissions. */
static int nova_files_receive_home_grant(int channel){
 if(!home_socket_trusted(channel))return -1;
 struct nova_files_grant g;struct nova_iovec io={&g,sizeof g};struct nova_msghdr m;
 union{struct nova_cmsghdr align;unsigned char bytes[NOVA_CMSG_SPACE(4*sizeof(int))];} control;
 nova_zero(&g,sizeof g);nova_zero(&m,sizeof m);nova_zero(&control,sizeof control);m.msg_iov=&io;m.msg_iovlen=1;m.msg_control=control.bytes;m.msg_controllen=sizeof control;
 long n=nova_recvmsg(channel,&m,NOVA_MSG_TRUNC|0x40000000);int fds[4]={-1,-1,-1,-1},valid=1;unsigned count=home_receive_rights(&m,control.bytes,fds,4,&valid);
 if(n!=sizeof g||m.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)||g.magic!=NOVA_FILES_GRANT_MAGIC||g.client_id!=3)valid=0;
 int shared=g.version==2&&g.scope==2;
 if((!shared&&(g.version!=1||g.scope!=1))||count!=(shared?2U:1U))valid=0;
 if(valid&&!nova_files_directory_fd_valid(fds[0]))valid=0;
 if(valid&&shared&&!home_socket_trusted(fds[1]))valid=0;
 if(!valid){for(unsigned i=0;i<count;i++)nova_close(fds[i]);return -1;}
 home_client.shared=shared;home_client.socket=shared?fds[1]:-1;home_client.sequence=0;home_client.details=-1;
 for(unsigned i=0;i<NOVA_HOME_DIRECTORY_HANDLES;i++){home_client.fds[i]=-1;home_client.tokens[i]=0;}
 home_client.fds[0]=fds[0];home_client.tokens[0]=1;return fds[0];
}
static unsigned home_token(int fd){for(unsigned i=0;i<NOVA_HOME_DIRECTORY_HANDLES;i++)if(home_client.fds[i]==fd)return home_client.tokens[i];return 0;}
static int home_request_bytes(unsigned op,unsigned token,const char *name,struct nova_home_directory_reply *reply,unsigned reply_bytes,int *pass){
 if(reply_bytes!=(op==NOVA_HOME_DIRECTORY_DETAILS?sizeof(struct nova_home_directory_details_reply):sizeof *reply))return -22;
 if(home_client.socket<0||!token||home_client.sequence==~0U)return -5;
 struct nova_home_directory_request req; nova_zero(&req,sizeof req);req.magic=NOVA_HOME_DIRECTORY_MAGIC;req.version=1;req.sequence=home_client.sequence+1;req.op=op;req.handle=token;
 if(name){unsigned i=0;for(;i<sizeof req.name&&name[i];i++)req.name[i]=name[i];if(i==sizeof req.name)return -22;}
 home_client.sequence=req.sequence;
 struct nova_iovec io={&req,sizeof req};struct nova_msghdr m;nova_zero(&m,sizeof m);m.msg_iov=&io;m.msg_iovlen=1;
 if(nova_sendmsg(home_client.socket,&m,0x40|0x4000)!=sizeof req){home_client_failed();return -5;}
 struct nova_pollfd poll={home_client.socket,NOVA_POLLIN,0};struct nova_timespec wait={0,250000000};
 if(nova_poll(&poll,1,&wait)<=0){home_client_failed();return -5;}
 union{struct nova_cmsghdr align;unsigned char bytes[NOVA_CMSG_SPACE(4*sizeof(int))];}control;
 nova_zero(&control,sizeof control);nova_zero(&m,sizeof m);nova_zero(reply,reply_bytes);io.iov_base=reply;io.iov_len=reply_bytes;m.msg_iov=&io;m.msg_iovlen=1;m.msg_control=control.bytes;m.msg_controllen=sizeof control;
 long n=nova_recvmsg(home_client.socket,&m,NOVA_MSG_TRUNC|0x40000000|0x40);int fds[4]={-1,-1,-1,-1},valid=1;unsigned count=home_receive_rights(&m,control.bytes,fds,4,&valid);
 unsigned want=(op==NOVA_HOME_DIRECTORY_OPEN||op==NOVA_HOME_DIRECTORY_PREVIEW)&&reply->status==0?1U:0U;
 if(n!=reply_bytes||m.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)||reply->magic!=NOVA_HOME_DIRECTORY_MAGIC||reply->version!=1||reply->sequence!=req.sequence||reply->op!=op||reply->reserved||count!=want||reply->status>0)valid=0;
 if(valid&&want&&op==NOVA_HOME_DIRECTORY_OPEN&&(!reply->handle||!nova_files_directory_fd_valid(fds[0])))valid=0;
 if(valid&&want&&op==NOVA_HOME_DIRECTORY_PREVIEW){
  struct nova_statx st;nova_zero(&st,sizeof st);long flags=nova_syscall3(NOVA_FILES_NR_FCNTL,fds[0],3,0);
  if(reply->kind!=8||reply->bytes>4096||flags<0||(flags&3)!=NOVA_O_RDONLY||nova_syscall5(NOVA_NR_statx,fds[0],(long)"",NOVA_AT_EMPTY_PATH,NOVA_STATX_TYPE|NOVA_STATX_SIZE,(long)&st)<0||(st.stx_mode&0170000)!=0100000||st.stx_size>4096)valid=0;
 }
 if(!valid){for(unsigned i=0;i<count;i++)nova_close(fds[i]);home_client_failed();return -5;}
 *pass=want?fds[0]:-1;return reply->status;
}
static int home_request(unsigned op,unsigned token,const char *name,struct nova_home_directory_reply *reply,int *pass){
 return home_request_bytes(op,token,name,reply,sizeof *reply,pass);
}
static long home_guest_seek(int fd,void *ctx){(void)ctx;if(home_client.shared&&home_client.socket<0)return -5;return nova_files_guest_seek(fd,0);}
static long home_guest_read(int fd,void *b,unsigned long n,void *ctx){(void)ctx;if(home_client.shared&&home_client.socket<0)return -5;return nova_files_guest_read(fd,b,n,0);}
static long home_guest_open(int fd,const char *name,void *ctx){
 (void)ctx;if(!home_client.shared)return nova_files_guest_open(fd,name,0);
 struct nova_home_directory_reply reply;int pass=-1;int status=home_request(NOVA_HOME_DIRECTORY_OPEN,home_token(fd),name,&reply,&pass);if(status<0)return status;
 for(unsigned i=0;i<NOVA_HOME_DIRECTORY_HANDLES;i++)if(home_client.fds[i]<0){home_client.fds[i]=pass;home_client.tokens[i]=reply.handle;return pass;}
 nova_close(pass);home_client_failed();return -24;
}
static long home_guest_close(int fd,void *ctx){
 (void)ctx;if(home_client.shared){for(unsigned i=0;i<NOVA_HOME_DIRECTORY_HANDLES;i++)if(home_client.fds[i]==fd){
  struct nova_home_directory_reply reply;int pass=-1;(void)home_request(NOVA_HOME_DIRECTORY_CLOSE,home_client.tokens[i],0,&reply,&pass);home_client.fds[i]=-1;home_client.tokens[i]=0;break;
 }}
 long result=nova_close(fd);if(home_client.shared){unsigned live=0;for(unsigned i=0;i<NOVA_HOME_DIRECTORY_HANDLES;i++)if(home_client.fds[i]>=0)live++;if(!live)home_client_failed();}return result;
}
static int nova_files_home_preview(int fd,const char *name,char *out,unsigned cap){
 if(!home_client.shared||!out||cap<4097)return -95;
 struct nova_home_directory_reply reply;int pass=-1;int rc=home_request(NOVA_HOME_DIRECTORY_PREVIEW,home_token(fd),name,&reply,&pass);if(rc<0)return rc;
 long n=nova_read(pass,out,4096);nova_close(pass);if(n<0)return (int)n;out[n]=0;
 for(long i=0;i<n;i++)if((unsigned char)out[i]<32&&out[i]!='\n'&&out[i]!='\t')out[i]='.';
 return (int)n;
}
static const struct nova_files_ops nova_files_home_ops={home_guest_seek,home_guest_read,home_guest_open,home_guest_close};
static int nova_files_home_metadata(int fd,const char *name,unsigned char type,struct nova_files_metadata *out){
 if(!home_client.shared)return nova_files_guest_metadata(fd,name,type,out);
 if(!out)return 0;nova_zero(out,sizeof *out);if(type!=4&&type!=8)return 0;
 if(home_client.details<0){struct nova_home_directory_reply caps;int capfd=-1;
  if(home_request(NOVA_HOME_DIRECTORY_STAMP,home_token(fd),0,&caps,&capfd)<0)return 0;
  if(caps.kind!=4||caps.handle&~NOVA_HOME_DIRECTORY_CAP_DETAILS){home_client_failed();return 0;}
  home_client.details=(caps.handle&NOVA_HOME_DIRECTORY_CAP_DETAILS)?1:0;
 }
 if(!home_client.details){struct nova_home_directory_reply legacy;int fdpass=-1;
  if(home_request(NOVA_HOME_DIRECTORY_META,home_token(fd),name,&legacy,&fdpass)<0||legacy.kind!=type)return 0;
  out->valid=1;out->bytes=legacy.bytes;return 1; /* Legacy worker: honest Unavailable POSIX fields. */
 }
 struct nova_home_directory_details_reply reply;int pass=-1;
 if(home_request_bytes(NOVA_HOME_DIRECTORY_DETAILS,home_token(fd),name,&reply.base,sizeof reply,&pass)<0||reply.base.kind!=type)return 0;
 if(reply.base.handle||reply.mode&~07777U){home_client_failed();return 0;}
 out->valid=1;out->bytes=reply.base.bytes;out->posix_valid=1;out->mode=reply.mode;out->uid=reply.uid;out->gid=reply.gid;return 1;
}
#endif
