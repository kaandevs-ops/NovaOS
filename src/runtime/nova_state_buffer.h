#ifndef NOVA_STATE_BUFFER_H
#define NOVA_STATE_BUFFER_H
#include "nova_runtime.h"

/* One single-threaded service snapshot at a time. The file format is unchanged.
 * Fixed storage avoids heap use; long snapshots flush bounded chunks. */
#define NOVA_STATE_BUFFER_BYTES 32768U
struct nova_state_buffer {
    int fd, active, failed;
    unsigned int used;
    unsigned long long writes, bytes;
    char data[NOVA_STATE_BUFFER_BYTES];
};
static struct nova_state_buffer g_nova_state;

static int nova_state_flush(void) {
    unsigned int done=0;
    while (done<g_nova_state.used && !g_nova_state.failed) {
        long n=nova_write(g_nova_state.fd,g_nova_state.data+done,g_nova_state.used-done);
        g_nova_state.writes++;
        if (n==-NOVA_EINTR) continue;
        if (n<=0 || (unsigned long)n>g_nova_state.used-done) {
            g_nova_state.failed=1; break;
        }
        done+=(unsigned int)n; g_nova_state.bytes+=(unsigned long)n;
    }
    g_nova_state.used=0;
    return !g_nova_state.failed;
}
static int nova_state_begin(int fd) {
    if (fd<0 || g_nova_state.active) return 0;
    g_nova_state.fd=fd;g_nova_state.active=1;g_nova_state.failed=0;g_nova_state.used=0;
    return 1;
}
static int nova_state_end(void) {
    if (!g_nova_state.active) return 0;
    int ok=nova_state_flush();g_nova_state.active=0;g_nova_state.fd=-1;
    return ok;
}
static void nova_state_puts(int fd,const char *text) {
    if (!g_nova_state.active || fd!=g_nova_state.fd) {nova_fd_puts(fd,text);return;}
    if (!text) return;
    while (*text && !g_nova_state.failed) {
        if (g_nova_state.used==NOVA_STATE_BUFFER_BYTES && !nova_state_flush()) return;
        g_nova_state.data[g_nova_state.used++]=*text++;
    }
}
static void nova_state_putln(int fd,const char *text) {
    nova_state_puts(fd,text);nova_state_puts(fd,"\n");
}
static void nova_state_put_u64(int fd,unsigned long value) {
    char reversed[24],text[24];unsigned int n=0,k=0;
    do {reversed[n++]=(char)('0'+value%10UL);value/=10UL;} while(value);
    while(n)text[k++]=reversed[--n];text[k]=0;nova_state_puts(fd,text);
}
#endif
