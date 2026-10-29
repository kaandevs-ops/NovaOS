#include "nova_runtime.h"
#include "nova_surface_protocol.h"

#if defined(NOVA_SECURITY_PROBE_READ_ONLY)
#define PROBE_CLIENT_ID 3U
#define PROBE_STATE_PATH "/run/nova/security-probe-read.state"
#define PROBE_MODE_READ_ONLY 1
#elif defined(NOVA_SECURITY_PROBE_NO_UI)
#define PROBE_CLIENT_ID 4U
#define PROBE_STATE_PATH "/run/nova/security-probe-no-ui.state"
#define PROBE_MODE_NO_UI 1
#elif defined(NOVA_SECURITY_PROBE_WRITE_ONLY)
#define PROBE_CLIENT_ID 5U
#define PROBE_STATE_PATH "/run/nova/security-probe-write.state"
#define PROBE_MODE_WRITE_ONLY 1
#elif defined(NOVA_SECURITY_PROBE_UNREGISTERED)
#define PROBE_CLIENT_ID 6U
#define PROBE_STATE_PATH "/run/nova/security-probe-unregistered.state"
#define PROBE_MODE_UNREGISTERED 1
#else
#error "Phase-16 security probe mode is required"
#endif

#define PROBE_SURFACE_ID (PROBE_CLIENT_ID * 100U)

struct probe_state {
    unsigned long connects;
    unsigned long hello_attempts;
    unsigned long spoof_attempts;
    unsigned long surface_creates;
    unsigned long input_text_requests;
    unsigned long tab_capture_requests;
    unsigned long clipboard_flag_requests;
    unsigned long clipboard_read_requests;
    unsigned long clipboard_write_requests;
};

static struct probe_state g_probe;

static void write_state(const char *stage) {
    long fd=nova_open_write(PROBE_STATE_PATH,0644);
    if(fd<0) return;
    nova_fd_putln((int)fd,"schema=1");
    nova_fd_puts((int)fd,"stage="); nova_fd_putln((int)fd,stage?stage:"UNKNOWN");
    nova_fd_puts((int)fd,"client_id="); nova_fd_put_u64((int)fd,PROBE_CLIENT_ID); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"pid="); nova_fd_put_u64((int)fd,(unsigned long)nova_getpid()); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"connects="); nova_fd_put_u64((int)fd,g_probe.connects); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"hello_attempts="); nova_fd_put_u64((int)fd,g_probe.hello_attempts); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"spoof_attempts="); nova_fd_put_u64((int)fd,g_probe.spoof_attempts); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"surface_creates="); nova_fd_put_u64((int)fd,g_probe.surface_creates); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"input_text_requests="); nova_fd_put_u64((int)fd,g_probe.input_text_requests); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"tab_capture_requests="); nova_fd_put_u64((int)fd,g_probe.tab_capture_requests); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"clipboard_flag_requests="); nova_fd_put_u64((int)fd,g_probe.clipboard_flag_requests); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"clipboard_read_requests="); nova_fd_put_u64((int)fd,g_probe.clipboard_read_requests); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"clipboard_write_requests="); nova_fd_put_u64((int)fd,g_probe.clipboard_write_requests); nova_fd_putln((int)fd,"");
    (void)nova_fsync((int)fd);
    (void)nova_close((int)fd);
}

static void sleep_ms(unsigned long ms) {
    struct nova_timespec ts;
    ts.tv_sec=(long)(ms/1000UL);
    ts.tv_nsec=(long)((ms%1000UL)*1000000UL);
    (void)nova_nanosleep(&ts,(struct nova_timespec *)0);
}

static void fill_addr(struct nova_sockaddr_un *addr) {
    const char path[]=NOVA_SURFACE_SOCKET_PATH;
    nova_zero(addr,sizeof(*addr));
    addr->sun_family=NOVA_AF_UNIX;
    for(unsigned long i=0;i<sizeof(path) && i<sizeof(addr->sun_path);i++) addr->sun_path[i]=path[i];
}

static int connect_compositor(void) {
    struct nova_sockaddr_un addr;
    fill_addr(&addr);
    for(unsigned int attempt=0U;attempt<80U;attempt++){
        long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET,0);
        if(fd>=0){
            if(nova_connect((int)fd,&addr,sizeof(addr))>=0){
                g_probe.connects++;
                return (int)fd;
            }
            (void)nova_close((int)fd);
        }
        sleep_ms(100UL);
    }
    return -1;
}

static int send_message(int fd,unsigned int opcode,unsigned int client_id,unsigned int surface_id,
                        unsigned int input_flags,const char *clipboard) {
    struct nova_surface_message m;
    nova_zero(&m,sizeof(m));
    m.magic=NOVA_SURFACE_PROTOCOL_MAGIC;
    m.version=NOVA_SURFACE_PROTOCOL_VERSION;
    m.opcode=opcode;
    m.size=sizeof(m);
    m.client_id=client_id;
    m.surface_id=surface_id;
    m.sequence=(unsigned int)(g_probe.hello_attempts+g_probe.surface_creates+
                             g_probe.input_text_requests+g_probe.tab_capture_requests+
                             g_probe.clipboard_flag_requests+g_probe.clipboard_read_requests+
                             g_probe.clipboard_write_requests+1UL);
    m.input_flags=input_flags;
    if(opcode==NOVA_SURFACE_OP_CREATE_SURFACE){
        m.x=48;
        m.y=96;
        m.width=128U;
        m.height=96U;
        m.z_index=50U;
    }
    if(clipboard){
        unsigned int n=0U;
        while(clipboard[n] && n+1U<NOVA_SURFACE_CLIPBOARD_BYTES){m.clipboard[n]=clipboard[n];n++;}
        m.clipboard_length=n;
    }
    return nova_write(fd,&m,sizeof(m))==(long)sizeof(m);
}

static int send_hello(int fd,unsigned int client_id) {
    g_probe.hello_attempts++;
    return send_message(fd,NOVA_SURFACE_OP_HELLO,client_id,0U,0U,(const char *)0);
}

#if defined(PROBE_MODE_READ_ONLY) || defined(PROBE_MODE_WRITE_ONLY)
static void keep_alive(int fd) {
    write_state("READY");
    for(;;){
        (void)send_message(fd,NOVA_SURFACE_OP_HEARTBEAT,PROBE_CLIENT_ID,0U,0U,(const char *)0);
        sleep_ms(500UL);
    }
}
#endif

int nova_main(void) {
    nova_zero(&g_probe,sizeof(g_probe));
    write_state("STARTING");

#if defined(PROBE_MODE_READ_ONLY)
    int fd=connect_compositor();
    if(fd<0){write_state("CONNECT_FAILED");return 2;}
    g_probe.spoof_attempts++;
    (void)send_hello(fd,99U);
    sleep_ms(150UL);
    (void)nova_close(fd);

    fd=connect_compositor();
    if(fd<0){write_state("RECONNECT_FAILED");return 3;}
    (void)send_hello(fd,PROBE_CLIENT_ID);
    sleep_ms(100UL);
    g_probe.surface_creates++;
    (void)send_message(fd,NOVA_SURFACE_OP_CREATE_SURFACE,PROBE_CLIENT_ID,PROBE_SURFACE_ID,0U,(const char *)0);
    g_probe.input_text_requests++;
    (void)send_message(fd,NOVA_SURFACE_OP_SET_INPUT_FLAGS,PROBE_CLIENT_ID,PROBE_SURFACE_ID,NOVA_SURFACE_INPUT_TEXT_ASCII,(const char *)0);
    g_probe.tab_capture_requests++;
    (void)send_message(fd,NOVA_SURFACE_OP_SET_INPUT_FLAGS,PROBE_CLIENT_ID,PROBE_SURFACE_ID,NOVA_SURFACE_INPUT_CAPTURE_TAB,(const char *)0);
    g_probe.clipboard_flag_requests++;
    (void)send_message(fd,NOVA_SURFACE_OP_SET_INPUT_FLAGS,PROBE_CLIENT_ID,PROBE_SURFACE_ID,NOVA_SURFACE_INPUT_CLIPBOARD,(const char *)0);
    g_probe.clipboard_write_requests++;
    (void)send_message(fd,NOVA_SURFACE_OP_SET_CLIPBOARD,PROBE_CLIENT_ID,PROBE_SURFACE_ID,0U,"PWN!");
    g_probe.clipboard_read_requests++;
    (void)send_message(fd,NOVA_SURFACE_OP_GET_CLIPBOARD,PROBE_CLIENT_ID,PROBE_SURFACE_ID,0U,(const char *)0);
    keep_alive(fd);
    return 0;
#elif defined(PROBE_MODE_WRITE_ONLY)
    int fd=connect_compositor();
    if(fd<0){write_state("CONNECT_FAILED");return 2;}
    (void)send_hello(fd,PROBE_CLIENT_ID);
    sleep_ms(100UL);
    g_probe.surface_creates++;
    (void)send_message(fd,NOVA_SURFACE_OP_CREATE_SURFACE,PROBE_CLIENT_ID,PROBE_SURFACE_ID,0U,(const char *)0);
    g_probe.clipboard_flag_requests++;
    (void)send_message(fd,NOVA_SURFACE_OP_SET_INPUT_FLAGS,PROBE_CLIENT_ID,PROBE_SURFACE_ID,NOVA_SURFACE_INPUT_CLIPBOARD,(const char *)0);
    g_probe.clipboard_read_requests++;
    (void)send_message(fd,NOVA_SURFACE_OP_GET_CLIPBOARD,PROBE_CLIENT_ID,PROBE_SURFACE_ID,0U,(const char *)0);
    g_probe.clipboard_write_requests++;
    (void)send_message(fd,NOVA_SURFACE_OP_SET_CLIPBOARD,PROBE_CLIENT_ID,PROBE_SURFACE_ID,0U,"SAFE");
    keep_alive(fd);
    return 0;
#else
    int fd=connect_compositor();
    if(fd<0){write_state("CONNECT_FAILED");return 2;}
    (void)send_hello(fd,PROBE_CLIENT_ID);
    sleep_ms(250UL);
    write_state("DONE");
    (void)nova_close(fd);
    return 0;
#endif
}
