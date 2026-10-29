#include "nova_runtime.h"
#include "nova_surface_protocol.h"

#if defined(NOVA_SANDBOX_PROBE_A)
#define PROBE_CLIENT_ID 3U
#define PROBE_APP_ID "nova.sandbox.probe.a"
#define PROBE_OWN_EXEC "/System/Apps/nova-sandbox-probe-a"
#define PROBE_OTHER_EXEC "/System/Apps/nova-sandbox-probe-b"
#elif defined(NOVA_SANDBOX_PROBE_B)
#define PROBE_CLIENT_ID 4U
#define PROBE_APP_ID "nova.sandbox.probe.b"
#define PROBE_OWN_EXEC "/System/Apps/nova-sandbox-probe-b"
#define PROBE_OTHER_EXEC "/System/Apps/nova-sandbox-probe-a"
#else
#error "Phase-17 sandbox probe mode is required"
#endif

#define PROBE_STATE_PATH "/Data/App/phase17-sandbox.state"
#define PROBE_PERSISTENT_PATH "/Data/App/persistent.marker"
#define PROBE_TMP_PATH "/tmp/phase17.marker"

struct probe_state {
    unsigned long connects;
    unsigned long reconnects;
    unsigned long hello_attempts;
    unsigned long connect_attempts;
    unsigned long connect_last_error;
    unsigned long socket_last_error;
    int own_exec_visible;
    int other_exec_hidden;
    int sensitive_paths_hidden;
    int control_sockets_hidden;
    int tmp_preexisting;
    int tmp_write_ok;
    int persistent_preexisting;
    unsigned long persistent_previous_pid;
    int persistent_write_ok;
    int af_inet_denied;
    int af_unix_stream_denied;
    int unknown_syscall_denied;
    int kill_syscall_denied;
    int wait4_syscall_denied;
};

static struct probe_state g_probe;

static void put_bool(int fd,const char *key,int value){
    nova_fd_puts(fd,key); nova_fd_puts(fd,"="); nova_fd_putln(fd,value?"1":"0");
}

static void write_state(const char *stage){
    long fd=nova_open_write(PROBE_STATE_PATH,0600);
    if(fd<0) return;
    nova_fd_putln((int)fd,"schema=1");
    nova_fd_puts((int)fd,"stage="); nova_fd_putln((int)fd,stage?stage:"UNKNOWN");
    nova_fd_puts((int)fd,"app_id="); nova_fd_putln((int)fd,PROBE_APP_ID);
    nova_fd_puts((int)fd,"client_id="); nova_fd_put_u64((int)fd,PROBE_CLIENT_ID); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"pid="); nova_fd_put_u64((int)fd,(unsigned long)nova_getpid()); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"connects="); nova_fd_put_u64((int)fd,g_probe.connects); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"reconnects="); nova_fd_put_u64((int)fd,g_probe.reconnects); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"hello_attempts="); nova_fd_put_u64((int)fd,g_probe.hello_attempts); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"connect_attempts="); nova_fd_put_u64((int)fd,g_probe.connect_attempts); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"connect_last_error="); nova_fd_put_u64((int)fd,g_probe.connect_last_error); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"socket_last_error="); nova_fd_put_u64((int)fd,g_probe.socket_last_error); nova_fd_putln((int)fd,"");
    put_bool((int)fd,"own_exec_visible",g_probe.own_exec_visible);
    put_bool((int)fd,"other_exec_hidden",g_probe.other_exec_hidden);
    put_bool((int)fd,"sensitive_paths_hidden",g_probe.sensitive_paths_hidden);
    put_bool((int)fd,"control_sockets_hidden",g_probe.control_sockets_hidden);
    put_bool((int)fd,"tmp_preexisting",g_probe.tmp_preexisting);
    put_bool((int)fd,"tmp_write_ok",g_probe.tmp_write_ok);
    put_bool((int)fd,"persistent_preexisting",g_probe.persistent_preexisting);
    nova_fd_puts((int)fd,"persistent_previous_pid="); nova_fd_put_u64((int)fd,g_probe.persistent_previous_pid); nova_fd_putln((int)fd,"");
    put_bool((int)fd,"persistent_write_ok",g_probe.persistent_write_ok);
    put_bool((int)fd,"af_inet_denied",g_probe.af_inet_denied);
    put_bool((int)fd,"af_unix_stream_denied",g_probe.af_unix_stream_denied);
    put_bool((int)fd,"unknown_syscall_denied",g_probe.unknown_syscall_denied);
    put_bool((int)fd,"kill_syscall_denied",g_probe.kill_syscall_denied);
    put_bool((int)fd,"wait4_syscall_denied",g_probe.wait4_syscall_denied);
    (void)nova_fsync((int)fd);
    (void)nova_close((int)fd);
}

static void sleep_ms(unsigned long ms){
    struct nova_timespec ts;
    ts.tv_sec=(long)(ms/1000UL);
    ts.tv_nsec=(long)((ms%1000UL)*1000000UL);
    (void)nova_nanosleep(&ts,(struct nova_timespec *)0);
}

static int path_exists(const char *path){
    long fd=nova_open_ro(path);
    if(fd<0) return 0;
    (void)nova_close((int)fd);
    return 1;
}

static int write_marker(const char *path,const char *text){
    long fd=nova_open_write(path,0600);
    if(fd<0) return 0;
    if(nova_write((int)fd,text,nova_strlen(text))!=(long)nova_strlen(text)){(void)nova_close((int)fd);return 0;}
    (void)nova_fsync((int)fd);
    (void)nova_close((int)fd);
    return 1;
}

static unsigned long read_pid_marker(const char *path){
    char buf[32];
    unsigned long value=0;
    long fd=nova_open_ro(path);
    if(fd<0) return 0;
    long n=nova_read((int)fd,buf,sizeof(buf)-1UL);
    (void)nova_close((int)fd);
    if(n<=0) return 0;
    buf[n]=0;
    for(long i=0;i<n;i++){
        char c=buf[i];
        if(c=='\n' || c=='\r' || c==0) break;
        if(c<'0' || c>'9') return 0;
        value=value*10UL+(unsigned long)(c-'0');
    }
    return value;
}

static int write_pid_marker(const char *path,unsigned long pid){
    long fd=nova_open_write(path,0600);
    if(fd<0) return 0;
    nova_fd_put_u64((int)fd,pid);
    nova_fd_putln((int)fd,"");
    (void)nova_fsync((int)fd);
    (void)nova_close((int)fd);
    return 1;
}

static void fill_addr(struct nova_sockaddr_un *addr,const char *path){
    nova_zero(addr,sizeof(*addr));
    addr->sun_family=NOVA_AF_UNIX;
    for(unsigned long i=0;path[i] && i+1UL<sizeof(addr->sun_path);i++) addr->sun_path[i]=path[i];
}

static int connect_path(const char *path,int type,unsigned long *socket_error,unsigned long *connect_error){
    struct nova_sockaddr_un addr;
    long fd=nova_socket(NOVA_AF_UNIX,type,0);
    if(socket_error) *socket_error=fd<0?(unsigned long)(-fd):0UL;
    if(connect_error) *connect_error=0UL;
    if(fd<0) return -1;
    fill_addr(&addr,path);
    long rc=nova_connect((int)fd,&addr,sizeof(addr));
    if(rc<0){
        if(connect_error) *connect_error=(unsigned long)(-rc);
        (void)nova_close((int)fd);
        return -1;
    }
    return (int)fd;
}

static int send_hello(int fd){
    struct nova_surface_message m;
    nova_zero(&m,sizeof(m));
    m.magic=NOVA_SURFACE_PROTOCOL_MAGIC;
    m.version=NOVA_SURFACE_PROTOCOL_VERSION;
    m.opcode=NOVA_SURFACE_OP_HELLO;
    m.size=sizeof(m);
    m.client_id=PROBE_CLIENT_ID;
    m.sequence=(unsigned int)(++g_probe.hello_attempts);
    return nova_write(fd,&m,sizeof(m))==(long)sizeof(m);
}

static int connect_compositor(void){
    for(unsigned int attempt=0U;attempt<100U;attempt++){
        g_probe.connect_attempts++;
        int fd=connect_path(NOVA_SURFACE_SOCKET_PATH,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK,
                            &g_probe.socket_last_error,&g_probe.connect_last_error);
        if(fd>=0){
            g_probe.connects++;
            if(send_hello(fd)) return fd;
            (void)nova_close(fd);
        }
        if(((attempt+1U)%10U)==0U) write_state("CONNECTING");
        sleep_ms(100UL);
    }
    return -1;
}

static int hidden_control_sockets(void){
    static const char *paths[]={
        "/run/nova/app-lifecycle.sock",
        "/run/nova/app-catalog.sock",
        "/run/nova/app-security.sock",
        "/run/nova/assets.sock",
        "/run/nova/desktop.sock",
        "/run/nova/compositor.sock",
    };
    for(unsigned long i=0;i<sizeof(paths)/sizeof(paths[0]);i++){
        int fd=connect_path(paths[i],NOVA_SOCK_SEQPACKET,(unsigned long *)0,(unsigned long *)0);
        if(fd>=0){(void)nova_close(fd);return 0;}
    }
    return 1;
}

static int hidden_sensitive_paths(void){
    static const char *paths[]={
        "/System/Trust/nova-root.der",
        "/System/Integrity/manifest.sha256",
        "/System/Services/services.list",
        "/Data/Nova",
        "/Recovery/recovery.conf",
        "/proc/self/status",
        "/sys",
        "/dev",
    };
    for(unsigned long i=0;i<sizeof(paths)/sizeof(paths[0]);i++) if(path_exists(paths[i])) return 0;
    return 1;
}

static void run_security_checks(void){
    long fd;
    int status=0;
    g_probe.own_exec_visible=path_exists(PROBE_OWN_EXEC);
    g_probe.other_exec_hidden=!path_exists(PROBE_OTHER_EXEC);
    g_probe.sensitive_paths_hidden=hidden_sensitive_paths();
    g_probe.control_sockets_hidden=hidden_control_sockets();

    g_probe.tmp_preexisting=path_exists(PROBE_TMP_PATH);
    g_probe.tmp_write_ok=write_marker(PROBE_TMP_PATH,"phase17-private-tmp\n");
    g_probe.persistent_preexisting=path_exists(PROBE_PERSISTENT_PATH);
    g_probe.persistent_previous_pid=read_pid_marker(PROBE_PERSISTENT_PATH);
    g_probe.persistent_write_ok=write_pid_marker(PROBE_PERSISTENT_PATH,(unsigned long)nova_getpid());

    fd=nova_socket(NOVA_AF_INET,NOVA_SOCK_STREAM,0);
    g_probe.af_inet_denied=(fd==-NOVA_EPERM);
    if(fd>=0) (void)nova_close((int)fd);
    fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_STREAM,0);
    g_probe.af_unix_stream_denied=(fd==-NOVA_EPERM);
    if(fd>=0) (void)nova_close((int)fd);
    g_probe.unknown_syscall_denied=(nova_syscall0(9999L)==-NOVA_EPERM);
    g_probe.kill_syscall_denied=(nova_kill(nova_getpid(),0)==-NOVA_EPERM);
    g_probe.wait4_syscall_denied=(nova_wait4(-1,&status,NOVA_WNOHANG)==-NOVA_EPERM);
}

int nova_main(void){
    int fd;
    nova_zero(&g_probe,sizeof(g_probe));
    run_security_checks();
    write_state("CHECKED");

    fd=connect_compositor();
    if(fd<0){write_state("CONNECT_FAILED");return 2;}
    write_state("READY");

    for(;;){
        struct nova_surface_message m;
        nova_zero(&m,sizeof(m));
        m.magic=NOVA_SURFACE_PROTOCOL_MAGIC;
        m.version=NOVA_SURFACE_PROTOCOL_VERSION;
        m.opcode=NOVA_SURFACE_OP_HEARTBEAT;
        m.size=sizeof(m);
        m.client_id=PROBE_CLIENT_ID;
        m.sequence=(unsigned int)(g_probe.hello_attempts+1UL);
        if(nova_write(fd,&m,sizeof(m))!=(long)sizeof(m)){
            (void)nova_close(fd);
            fd=connect_compositor();
            if(fd<0){write_state("RECONNECT_FAILED");return 3;}
            g_probe.reconnects++;
            write_state("READY");
        }
        sleep_ms(250UL);
    }
}
