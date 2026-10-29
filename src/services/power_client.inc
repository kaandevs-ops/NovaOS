/* Narrow compositor -> supervised PID1 request; never reboot() in desktop. */
static unsigned int g_power_status,g_power_focus;static int g_power_fd=-1;static unsigned int g_power_seq;static unsigned long long g_power_until;
static void r46_power_disconnect(void){if(g_power_fd>=0)nova_close(g_power_fd);g_power_fd=-1;}
static void r46_power_request(void){
 if(g_power_status==1||g_power_status==2)return;g_power_status=3;long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK|02000000,0);if(fd<0)return;
 struct nova_sockaddr_un a;nova_zero(&a,sizeof a);a.sun_family=NOVA_AF_UNIX;const char*path=NOVA_POWER_PATH;unsigned int i=0;while(path[i]&&i+1<sizeof a.sun_path){a.sun_path[i]=path[i];i++;}
 if(nova_connect((int)fd,&a,sizeof a)<0){nova_close((int)fd);return;}struct nova_ucred peer;unsigned int len=sizeof peer;
 if(nova_getsockopt((int)fd,NOVA_SOL_SOCKET,NOVA_SO_PEERCRED,&peer,&len)<0||len!=sizeof peer||peer.uid||peer.pid!=1){nova_close((int)fd);return;}
 struct nova_power_message q={NOVA_POWER_MAGIC,1,sizeof q,++g_power_seq,1,0};if(!q.sequence)q.sequence=++g_power_seq;struct nova_iovec io={&q,sizeof q};struct nova_msghdr msg;nova_zero(&msg,sizeof msg);msg.msg_iov=&io;msg.msg_iovlen=1;
 if(nova_sendmsg((int)fd,&msg,NOVA_MSG_DONTWAIT|NOVA_MSG_NOSIGNAL)!=sizeof q){nova_close((int)fd);return;}g_power_fd=(int)fd;g_power_status=1;g_power_until=nova_render_clock(1)+2000000000ULL;
}
static void r46_power_tick(void){if(g_power_fd<0)return;struct nova_power_message q;struct nova_iovec io={&q,sizeof q};struct nova_msghdr m;nova_zero(&m,sizeof m);m.msg_iov=&io;m.msg_iovlen=1;long n=nova_recvmsg(g_power_fd,&m,NOVA_MSG_DONTWAIT|NOVA_MSG_CMSG_CLOEXEC);
 if(n==-NOVA_EAGAIN&&nova_render_clock(1)<g_power_until)return;g_power_status=n==sizeof q&&!(m.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC))&&nova_power_valid(&q)&&q.sequence==g_power_seq?2U:3U;r46_power_disconnect();g_scene.visual_generation++;}
static void r46_power_draw(struct nova_ui_canvas c,struct nova_ui_palette p,int x,int y,int w,int h){
 nova_ui_round(c,x,y,w,h,14,p.paper);nova_ui_text(c,x+20,y+38,NOVA_UI_TITLE,1,p.ink,"Power off NovaOS?",x+16,y+12,w-32,36);
 nova_ui_text(c,x+20,y+86,NOVA_UI_BODY,1,p.ink,"Save your work before continuing.",x+16,y+60,w-32,38);
 const char*state=g_power_status==1?"Request sent to system manager":g_power_status==2?"Stopping services and syncing data":g_power_status==3?"Unavailable: shutdown was not confirmed":"The system will stop services and sync data.";
 nova_ui_text(c,x+20,y+130,NOVA_UI_CAPTION,1,p.muted,state,x+16,y+100,w-32,42);
 nova_ui_button(c,x+20,y+h-64,(w-52)/2,40,p,(g_power_focus==0?NOVA_UI_FOCUSED:0)|(g_power_status==1||g_power_status==2?NOVA_UI_DISABLED:0));nova_ui_text(c,x+32,y+h-38,NOVA_UI_BODY,1,p.ink,"Cancel",x+20,y+h-64,(w-52)/2,40);
 nova_ui_button(c,x+32+(w-52)/2,y+h-64,(w-52)/2,40,p,(g_power_focus==1?NOVA_UI_FOCUSED:0)|(g_power_status==1||g_power_status==2?NOVA_UI_DISABLED:0));nova_ui_text(c,x+44+(w-52)/2,y+h-38,NOVA_UI_BODY,1,p.ink,"Power off",x+32+(w-52)/2,y+h-64,(w-52)/2,40);
}
