#include "nova_runtime.h"
#include "nova_asset_protocol.h"
#include "nova_sha256.h"

#define NOVA_ASSET_CLIENTS 4U
#define NOVA_BUILTIN_ICON_PACKED_BYTES 72U
#define NOVA_BUILTIN_ICON_PIXELS 576U

struct builtin_asset {
    const char *asset_id;
    unsigned int width;
    unsigned int height;
    const unsigned char *packed_bits;
    const unsigned char *digest;
};

static const unsigned char g_icon_a_bits[72] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xe0,
    0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x11, 0x00, 0x88, 0x11, 0x00, 0x88,
    0x11, 0x00, 0x88, 0x10, 0x81, 0x08, 0x10, 0x81, 0x08, 0x10, 0x81, 0x08,
    0x10, 0x42, 0x08, 0x10, 0x7e, 0x08, 0x10, 0x42, 0x08, 0x10, 0x24, 0x08,
    0x10, 0x24, 0x08, 0x10, 0x24, 0x08, 0x10, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x07, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const unsigned char g_icon_a_digest[32] = {
    0xef, 0x8d, 0x9a, 0xff, 0x4b, 0x83, 0x06, 0xd2, 0xca, 0xee, 0x03, 0x4d, 0xef, 0xdc, 0x26, 0x7a,
    0x35, 0x9e, 0x06, 0x22, 0x8b, 0x52, 0xe5, 0x4a, 0x1a, 0x5f, 0xc6, 0x8a, 0x54, 0x03, 0xd8, 0x1a
};
static const unsigned char g_icon_b_bits[72] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xe0,
    0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x10, 0xfc, 0x08, 0x10, 0x82, 0x08,
    0x10, 0x82, 0x08, 0x10, 0x82, 0x08, 0x10, 0x82, 0x08, 0x10, 0xfc, 0x08,
    0x10, 0x81, 0x08, 0x10, 0x81, 0x08, 0x10, 0x81, 0x08, 0x10, 0x81, 0x08,
    0x10, 0x81, 0x08, 0x10, 0xfc, 0x08, 0x10, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x07, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const unsigned char g_icon_b_digest[32] = {
    0x12, 0x60, 0xc6, 0x58, 0x02, 0x60, 0xbb, 0x97, 0xc0, 0x27, 0x91, 0xc2, 0x18, 0xdf, 0x5e, 0x9d,
    0xd9, 0x87, 0x74, 0x93, 0xa4, 0xb4, 0xde, 0x80, 0x25, 0xc1, 0x31, 0xb7, 0xf7, 0xa0, 0xdd, 0xb1
};

static const struct builtin_asset g_assets[] = {
    { "nova.icon.demo.a", 24U, 24U, g_icon_a_bits, g_icon_a_digest },
    { "nova.icon.demo.b", 24U, 24U, g_icon_b_bits, g_icon_b_digest },
};

static unsigned long g_requests;
static unsigned long g_replies;
static unsigned long g_rejected;
static unsigned long g_connections;
static unsigned long g_disconnects;

static void copy_string(char *dst, unsigned long cap, const char *src) {
    if(!dst || !cap) return;
    unsigned long i=0;
    while(src && src[i] && i+1UL<cap){ dst[i]=src[i]; i++; }
    dst[i]='\0';
    for(i++;i<cap;i++) dst[i]='\0';
}

static int asset_id_valid(const char id[NOVA_ASSET_ID_BYTES], int allow_empty) {
    unsigned long nul=NOVA_ASSET_ID_BYTES;
    for(unsigned long i=0;i<NOVA_ASSET_ID_BYTES;i++) if(id[i]=='\0'){ nul=i; break; }
    if(nul==NOVA_ASSET_ID_BYTES) return 0;
    if(nul==0UL) return allow_empty;
    for(unsigned long i=0;i<nul;i++){
        char c=id[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-')) return 0;
    }
    return 1;
}

static const struct builtin_asset *find_asset(const char *id) {
    if(!id) return (const struct builtin_asset *)0;
    for(unsigned long i=0;i<sizeof(g_assets)/sizeof(g_assets[0]);i++)
        if(nova_streq(g_assets[i].asset_id,id)) return &g_assets[i];
    return (const struct builtin_asset *)0;
}

static unsigned char expanded_byte(const struct builtin_asset *asset, unsigned int offset) {
    if(!asset || offset>=asset->width*asset->height) return 0U;
    unsigned char packed=asset->packed_bits[offset>>3];
    unsigned int shift=7U-(offset&7U);
    return ((packed>>shift)&1U) ? 0xffU : 0x00U;
}

static void fill_addr(struct nova_sockaddr_un *addr) {
    nova_zero(addr,sizeof(*addr));
    addr->sun_family=NOVA_AF_UNIX;
    copy_string(addr->sun_path,sizeof(addr->sun_path),NOVA_ASSET_SOCKET_PATH);
}

static void digest_hex(const unsigned char digest[32], char out[65]) {
    nova_sha256_hex(digest,out);
}

static void write_state(unsigned int connected) {
    long fd=nova_open_write(NOVA_ASSET_STATE_PATH,0644);
    if(fd<0) return;
    nova_fd_putln((int)fd,"schema=1");
    nova_fd_puts((int)fd,"protocol="); nova_fd_put_u64((int)fd,NOVA_ASSET_PROTOCOL_VERSION); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"registry_count="); nova_fd_put_u64((int)fd,sizeof(g_assets)/sizeof(g_assets[0])); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"connected_clients="); nova_fd_put_u64((int)fd,connected); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"requests="); nova_fd_put_u64((int)fd,g_requests); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"replies="); nova_fd_put_u64((int)fd,g_replies); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"rejected="); nova_fd_put_u64((int)fd,g_rejected); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"connections="); nova_fd_put_u64((int)fd,g_connections); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"disconnects="); nova_fd_put_u64((int)fd,g_disconnects); nova_fd_putln((int)fd,"");
    for(unsigned long i=0;i<sizeof(g_assets)/sizeof(g_assets[0]);i++){
        char hex[65]; digest_hex(g_assets[i].digest,hex);
        nova_fd_puts((int)fd,"asset"); nova_fd_put_u64((int)fd,i+1UL); nova_fd_puts((int)fd,"_id="); nova_fd_putln((int)fd,g_assets[i].asset_id);
        nova_fd_puts((int)fd,"asset"); nova_fd_put_u64((int)fd,i+1UL); nova_fd_puts((int)fd,"_format=a8-mask"); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"asset"); nova_fd_put_u64((int)fd,i+1UL); nova_fd_puts((int)fd,"_width="); nova_fd_put_u64((int)fd,g_assets[i].width); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"asset"); nova_fd_put_u64((int)fd,i+1UL); nova_fd_puts((int)fd,"_height="); nova_fd_put_u64((int)fd,g_assets[i].height); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"asset"); nova_fd_put_u64((int)fd,i+1UL); nova_fd_puts((int)fd,"_sha256="); nova_fd_putln((int)fd,hex);
    }
    nova_close((int)fd);
}

static int request_valid(const struct nova_asset_request *r) {
    if(!r) return 0;
    if(r->magic!=NOVA_ASSET_PROTOCOL_MAGIC || r->version!=NOVA_ASSET_PROTOCOL_VERSION || r->size!=sizeof(*r) || r->flags!=0U) return 0;
    if(r->opcode==NOVA_ASSET_OP_HELLO) return r->offset==0U && r->length==0U && asset_id_valid(r->asset_id,1) && r->asset_id[0]=='\0';
    if(r->opcode==NOVA_ASSET_OP_GET_INFO) return r->offset==0U && r->length==0U && asset_id_valid(r->asset_id,0);
    if(r->opcode==NOVA_ASSET_OP_READ) return r->length>0U && r->length<=NOVA_ASSET_CHUNK_BYTES && asset_id_valid(r->asset_id,0);
    return 0;
}

static void fill_reply(struct nova_asset_reply *reply,const struct nova_asset_request *request,
                       unsigned int opcode,unsigned int result,const struct builtin_asset *asset) {
    nova_zero(reply,sizeof(*reply));
    reply->magic=NOVA_ASSET_PROTOCOL_MAGIC;
    reply->version=NOVA_ASSET_PROTOCOL_VERSION;
    reply->opcode=opcode;
    reply->size=sizeof(*reply);
    reply->request_id=request?request->request_id:0U;
    reply->result=result;
    reply->registry_count=(unsigned int)(sizeof(g_assets)/sizeof(g_assets[0]));
    if(!asset) return;
    reply->format=NOVA_ASSET_FORMAT_A8_MASK;
    reply->asset_flags=NOVA_ASSET_FLAG_IMMUTABLE|NOVA_ASSET_FLAG_BUILTIN;
    reply->width=asset->width;
    reply->height=asset->height;
    reply->total_bytes=asset->width*asset->height;
    nova_copy(reply->digest,asset->digest,NOVA_ASSET_DIGEST_BYTES);
    copy_string(reply->asset_id,sizeof(reply->asset_id),asset->asset_id);
}

static int send_reply(int fd,const struct nova_asset_reply *reply) {
    long n=nova_write(fd,reply,sizeof(*reply));
    if(n==(long)sizeof(*reply)){ g_replies++; return 0; }
    return -1;
}

static int process_packet(int fd) {
    struct nova_asset_request request;
    struct nova_iovec iov;
    struct nova_msghdr msg;
    nova_zero(&request,sizeof(request)); nova_zero(&iov,sizeof(iov)); nova_zero(&msg,sizeof(msg));
    iov.iov_base=&request; iov.iov_len=sizeof(request); msg.msg_iov=&iov; msg.msg_iovlen=1;
    long n=nova_recvmsg(fd,&msg,0);
    if(n==-NOVA_EAGAIN || n==-NOVA_EINTR) return 0;
    if(n<=0) return -1;
    g_requests++;

    struct nova_asset_reply reply;
    if(n!=(long)sizeof(request) || (msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)) || !request_valid(&request)){
        g_rejected++;
        fill_reply(&reply,&request,NOVA_ASSET_OP_ERROR,NOVA_ASSET_RESULT_BAD_REQUEST,(const struct builtin_asset *)0);
        return send_reply(fd,&reply)<0 ? -1 : 0;
    }
    if(request.opcode==NOVA_ASSET_OP_HELLO){
        fill_reply(&reply,&request,NOVA_ASSET_OP_HELLO_REPLY,NOVA_ASSET_RESULT_OK,(const struct builtin_asset *)0);
        return send_reply(fd,&reply)<0 ? -1 : 0;
    }
    const struct builtin_asset *asset=find_asset(request.asset_id);
    if(!asset){
        g_rejected++;
        fill_reply(&reply,&request,NOVA_ASSET_OP_ERROR,NOVA_ASSET_RESULT_UNKNOWN_ASSET,(const struct builtin_asset *)0);
        return send_reply(fd,&reply)<0 ? -1 : 0;
    }
    if(request.opcode==NOVA_ASSET_OP_GET_INFO){
        fill_reply(&reply,&request,NOVA_ASSET_OP_INFO,NOVA_ASSET_RESULT_OK,asset);
        return send_reply(fd,&reply)<0 ? -1 : 0;
    }
    unsigned int total=asset->width*asset->height;
    if(request.offset>=total || request.length>total-request.offset){
        g_rejected++;
        fill_reply(&reply,&request,NOVA_ASSET_OP_ERROR,NOVA_ASSET_RESULT_RANGE,asset);
        return send_reply(fd,&reply)<0 ? -1 : 0;
    }
    fill_reply(&reply,&request,NOVA_ASSET_OP_DATA,NOVA_ASSET_RESULT_OK,asset);
    reply.offset=request.offset; reply.chunk_bytes=request.length;
    for(unsigned int i=0;i<request.length;i++) reply.payload[i]=expanded_byte(asset,request.offset+i);
    return send_reply(fd,&reply)<0 ? -1 : 0;
}

static unsigned int connected_count(const int clients[NOVA_ASSET_CLIENTS]) {
    unsigned int count=0U;
    for(unsigned int i=0;i<NOVA_ASSET_CLIENTS;i++) if(clients[i]>=0) count++;
    return count;
}

static void disconnect_client(int clients[NOVA_ASSET_CLIENTS],unsigned int slot) {
    if(slot>=NOVA_ASSET_CLIENTS || clients[slot]<0) return;
    nova_close(clients[slot]); clients[slot]=-1; g_disconnects++;
}

static void accept_clients(int listener,int clients[NOVA_ASSET_CLIENTS]) {
    for(;;){
        long fd=nova_accept4(listener,NOVA_SOCK_NONBLOCK);
        if(fd==-NOVA_EAGAIN || fd==-NOVA_EINTR) return;
        if(fd<0) return;
        int placed=0;
        for(unsigned int i=0;i<NOVA_ASSET_CLIENTS;i++) if(clients[i]<0){ clients[i]=(int)fd; placed=1; g_connections++; break; }
        if(!placed) nova_close((int)fd);
    }
}

int nova_main(void) {
    _Static_assert(sizeof(g_assets)/sizeof(g_assets[0])<=NOVA_ASSET_MAX_ENTRIES,"builtin asset registry exceeds protocol bound");
    _Static_assert(NOVA_BUILTIN_ICON_PIXELS<=NOVA_ASSET_MAX_BYTES,"builtin icon exceeds asset byte bound");
    _Static_assert(NOVA_BUILTIN_ICON_PACKED_BYTES*8U==NOVA_BUILTIN_ICON_PIXELS,"packed icon geometry mismatch");

    struct nova_sockaddr_un addr; fill_addr(&addr);
    (void)nova_unlink(NOVA_ASSET_SOCKET_PATH);
    long listener=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK,0);
    if(listener<0){ nova_putln("[nova-assets] startup=FAILED reason=socket"); return 1; }
    if(nova_bind((int)listener,&addr,sizeof(addr))<0 || nova_listen((int)listener,NOVA_ASSET_CLIENTS)<0){
        nova_putln("[nova-assets] startup=FAILED reason=bind-listen"); nova_close((int)listener); return 1;
    }
    int clients[NOVA_ASSET_CLIENTS]; for(unsigned int i=0;i<NOVA_ASSET_CLIENTS;i++) clients[i]=-1;
    write_state(0U);
    nova_putln("[nova-assets] stage=LISTENING protocol=1 registry=builtin-immutable transport=bounded-chunk format=a8-mask");

    for(;;){
        struct nova_pollfd fds[1U+NOVA_ASSET_CLIENTS];
        nova_zero(fds,sizeof(fds));
        fds[0].fd=(int)listener; fds[0].events=NOVA_POLLIN;
        for(unsigned int i=0;i<NOVA_ASSET_CLIENTS;i++){ fds[1U+i].fd=clients[i]; fds[1U+i].events=NOVA_POLLIN; }
        struct nova_timespec timeout; timeout.tv_sec=1; timeout.tv_nsec=0;
        long rc=nova_poll(fds,1U+NOVA_ASSET_CLIENTS,&timeout);
        if(rc<0 && rc!=-NOVA_EINTR) continue;
        if(fds[0].revents&NOVA_POLLIN) accept_clients((int)listener,clients);
        for(unsigned int i=0;i<NOVA_ASSET_CLIENTS;i++){
            if(clients[i]<0) continue;
            short rev=fds[1U+i].revents;
            if(rev&(NOVA_POLLERR|NOVA_POLLHUP)) disconnect_client(clients,i);
            else if((rev&NOVA_POLLIN) && process_packet(clients[i])<0) disconnect_client(clients,i);
        }
        write_state(connected_count(clients));
    }
}
