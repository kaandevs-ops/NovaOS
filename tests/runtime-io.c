#include "nova_runtime.h"

static int exact(const char *a, const char *b) { return nova_streq(a, b); }

int nova_main(void) {
    static const char path[] = "/tmp/novaos-runtime-io-regression.txt";
    static const char expected[] =
        "ts=3 level=INFO component=trust event=signature_verified "
        "message=signed integrity manifest verified by pinned NovaOS root\n";

    long fd = nova_open_write(path, 0600);
    if (fd < 0) return 10;
    if (nova_fd_write_all((int)fd, expected, nova_strlen(expected)) < 0) {
        nova_close((int)fd);
        return 11;
    }
    nova_close((int)fd);

    char actual[256];
    fd = nova_open_ro(path);
    if (fd < 0) return 12;
    nova_size_t used = 0;
    for (;;) {
        long n = nova_read((int)fd, actual + used, sizeof(actual) - used - 1);
        if (n == -NOVA_EINTR) continue;
        if (n < 0) { nova_close((int)fd); return 13; }
        if (n == 0) break;
        used += (nova_size_t)n;
        if (used + 1 >= sizeof(actual)) { nova_close((int)fd); return 14; }
    }
    nova_close((int)fd);
    actual[used] = '\0';
    (void)nova_unlink(path);

    if (!exact(actual, expected)) return 15;
    nova_putln("Runtime I/O exact-string regression: PASS");
    return 0;
}
