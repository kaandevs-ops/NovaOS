/* Compositor-owned, nonblocking, bounded read-only novad client. */
static struct nova_overview_reply g_overview;
static int g_overview_fd=-1;
static unsigned int g_overview_ready,g_overview_pending,g_overview_sequence;
static unsigned long long g_overview_next,g_overview_deadline,g_overview_local_ns,g_overview_last_request;
static void r23_overview_disconnect(void){if(g_overview_fd>=0)nova_close(g_overview_fd);g_overview_fd=-1;g_overview_pending=0;g_overview_ready=0;}
static int r23_overview_accept(const struct nova_overview_reply*q){if(!g_overview_pending||!nova_overview_reply_valid(q,g_overview_sequence))return 0;g_overview=*q;g_overview_pending=0;g_overview_ready=1;g_scene.visual_generation++;return 1;}
static void r23_overview_tick(unsigned long long now){
 if(g_r7_controls_visible!=1||g_settings_page!=1){r23_overview_disconnect();return;}
 if(g_overview_pending){struct nova_overview_reply q;struct nova_iovec io={&q,sizeof(q)};struct nova_msghdr msg;nova_zero(&q,sizeof(q));nova_zero(&msg,sizeof(msg));msg.msg_iov=&io;msg.msg_iovlen=1;long n=nova_recvmsg(g_overview_fd,&msg,NOVA_MSG_DONTWAIT|NOVA_MSG_CMSG_CLOEXEC);
  if(n>0){if(n!=sizeof(q)||(msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC))||!r23_overview_accept(&q)){r23_overview_disconnect();g_scene.visual_generation++;}}
  else if((n<0&&n!=-NOVA_EAGAIN&&n!=-NOVA_EINTR)||n==0||now>=g_overview_deadline){r23_overview_disconnect();g_scene.visual_generation++;}
 }
 if(now<g_overview_next||g_overview_pending)return;if(g_overview_last_request&&(now<g_overview_last_request||now-g_overview_last_request<1000000000ULL)){g_overview_next=g_overview_last_request+1000000000ULL;return;}g_overview_last_request=now;g_overview_ready=0;g_overview_next=now+2000000000ULL;g_overview_local_ns=now;g_scene.visual_generation++;
 if(g_overview_fd<0){struct nova_sockaddr_un addr;nova_zero(&addr,sizeof(addr));addr.sun_family=NOVA_AF_UNIX;const char*path=NOVA_OVERVIEW_PATH;for(unsigned int i=0;path[i]&&i+1<sizeof(addr.sun_path);i++)addr.sun_path[i]=path[i];long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK|02000000,0);if(fd<0)return;
  struct nova_ucred peer;unsigned int len=sizeof(peer);nova_zero(&peer,sizeof(peer));if(nova_connect((int)fd,&addr,sizeof(addr))<0||nova_getsockopt((int)fd,NOVA_SOL_SOCKET,NOVA_SO_PEERCRED,&peer,&len)<0||len!=sizeof(peer)||peer.uid!=0||peer.pid!=1){nova_close((int)fd);return;}g_overview_fd=(int)fd;
 }
 g_overview_sequence=g_overview_sequence==0xffffffffU?1:g_overview_sequence+1;struct nova_overview_request q={NOVA_OVERVIEW_MAGIC,1,sizeof(q),g_overview_sequence,0,0};struct nova_iovec io={&q,sizeof(q)};struct nova_msghdr msg;nova_zero(&msg,sizeof(msg));msg.msg_iov=&io;msg.msg_iovlen=1;
 if(nova_sendmsg(g_overview_fd,&msg,NOVA_MSG_DONTWAIT|NOVA_MSG_NOSIGNAL)!=sizeof(q)){r23_overview_disconnect();return;}g_overview_pending=1;g_overview_deadline=now+500000000ULL;
}
static void r23_number(unsigned long long n,char*out){char reverse[24];unsigned int k=0,j=0;do{reverse[k++]=(char)('0'+n%10);n/=10;}while(n);while(k)out[j++]=reverse[--k];out[j]=0;}
static const char*r23_trust_label(unsigned int value){return value==1?"Verified by novad":value==2?"Not verified":"Unavailable";}
static void r23_overview_row(struct nova_ui_canvas c,struct nova_ui_palette p,int x,int y,int width,const char*label,const char*value){char l[64],v[64];nova_ui_ellipsis(label,l,sizeof(l),NOVA_UI_CAPTION,(unsigned int)(width/2-8));nova_ui_ellipsis(value,v,sizeof(v),NOVA_UI_CAPTION,(unsigned int)(width/2));nova_ui_text(c,x,y,NOVA_UI_CAPTION,1,p.muted,l,x,y-22,(unsigned int)width/2-8,28);int n=(int)nova_ui_measure(v,NOVA_UI_CAPTION,1);nova_ui_text(c,x+width-n,y,NOVA_UI_CAPTION,1,p.ink,v,x+width/2,y-22,(unsigned int)width/2,28);nova_ui_line(c,x,y+12,x+width,y+12,p.line);}
static void r23_overview_draw(struct nova_ui_canvas c,struct nova_ui_palette p,int x,int y,unsigned int pw,unsigned int ph){
 int left=x+154,wide=(int)pw-176;char value[48];nova_ui_text(c,left,y+42,NOVA_UI_TITLE,1,p.ink,"System Overview",left,y+16,(unsigned int)wide,36);
 nova_ui_text(c,left,y+70,NOVA_UI_CAPTION,1,p.muted,"NovaOS " NOVA_VERSION " / " NOVA_NATIVE_ARCH,left,y+48,(unsigned int)wide,30);
 char build[80];nova_ui_ellipsis(NOVA_BUILD_ID,build,sizeof(build),NOVA_UI_CAPTION,(unsigned int)wide);nova_ui_text(c,left,y+94,NOVA_UI_CAPTION,1,p.muted,build,left,y+72,(unsigned int)wide,28);
 unsigned long long ns=g_overview_local_ns?g_overview_local_ns:nova_render_clock(1);if(ns)r23_number(ns/1000000000ULL,value);else{const char*t="Unavailable";unsigned int n=0;while(t[n]){value[n]=t[n];n++;}value[n]=0;}r23_overview_row(c,p,left,y+128,wide,"Kernel uptime (seconds)",value);
 r23_number(client_connection_count(),value);r23_overview_row(c,p,left,y+164,wide,"Authenticated apps",value);
 unsigned long long bytes=0;for(unsigned int i=0;i<NOVA_SURFACE_MAX_CLIENT_BUFFERS;i++)if(g_client_buffers[i].used)bytes+=g_client_buffers[i].bytes;r23_number(bytes/1024ULL,value);r23_overview_row(c,p,left,y+200,wide,"Client buffers (KiB)",value);
 if(g_overview_ready){char a[24],b[24];r23_number(g_overview.services_running,a);r23_number(g_overview.services_total,b);unsigned int n=0;while(a[n]){value[n]=a[n];n++;}value[n++]='/';for(unsigned int k=0;b[k];k++)value[n++]=b[k];value[n]=0;}else{const char*t=g_overview_pending?"Loading":"Unavailable";unsigned int n=0;while(t[n]){value[n]=t[n];n++;}value[n]=0;}
 r23_overview_row(c,p,left,y+236,wide,"Services running",value);r23_overview_row(c,p,left,y+272,wide,"Secure Boot",g_overview_ready?r23_trust_label(g_overview.secure_boot):"Unavailable");
 if(ph>=350)r23_overview_row(c,p,left,y+308,wide,"TPM / dm-verity",g_overview_ready&&(g_overview.tpm==2||g_overview.verity==2)?"Not verified":g_overview_ready&&g_overview.tpm==1&&g_overview.verity==1?"Ready / enforced":"Unavailable");
 if(ph<400)nova_ui_text(c,left,y+(int)ph-16,NOVA_UI_CAPTION,1,p.muted,"R: refresh / F1: appearance",left,y+(int)ph-38,(unsigned int)wide,28);
 if(ph>=400){nova_ui_text(c,left,y+342,NOVA_UI_CAPTION,1,p.muted,"CPU / total RAM: unavailable",left,y+320,(unsigned int)wide,28);nova_ui_button(c,left,y+362,112,34,p,NOVA_UI_FOCUSED);nova_ui_text(c,left+12,y+385,NOVA_UI_CAPTION,1,p.ink,"Refresh status",left,y+362,112,34);}
}
