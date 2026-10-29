#include "nova_runtime.h"

nova_size_t nova_strlen(const char *s) {
    nova_size_t n = 0;
    while (s && s[n]) n++;
    return n;
}

int nova_streq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

int nova_startswith(const char *s, const char *prefix) {
    if (!s || !prefix) return 0;
    while (*prefix) {
        if (*s++ != *prefix++) return 0;
    }
    return 1;
}

const char *nova_skip_spaces(const char *s) {
    while (s && (*s == ' ' || *s == '\t')) s++;
    return s;
}

int nova_fd_write_all(int fd, const void *buf, nova_size_t len) {
    const unsigned char *p = (const unsigned char *)buf;
    while (len) {
        long n = nova_write(fd, p, len);
        if (n == -NOVA_EINTR) continue;
        if (n <= 0) return -1;
        p += (nova_size_t)n;
        len -= (nova_size_t)n;
    }
    return 0;
}

void nova_fd_puts(int fd, const char *s) {
    (void)nova_fd_write_all(fd, s, nova_strlen(s));
}

void nova_fd_putln(int fd, const char *s) {
    nova_fd_puts(fd, s);
    nova_fd_puts(fd, "\n");
}

void nova_puts(const char *s) { nova_fd_puts(NOVA_STDOUT, s); }
void nova_putln(const char *s) { nova_fd_putln(NOVA_STDOUT, s); }

void nova_fd_put_u64(int fd, unsigned long value) {
    char reverse[32];
    char out[32];
    nova_size_t n = 0;
    nova_size_t out_n = 0;
    if (value == 0) {
        (void)nova_fd_write_all(fd, "0", 1);
        return;
    }
    while (value && n < sizeof(reverse)) {
        reverse[n++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (n) out[out_n++] = reverse[--n];
    (void)nova_fd_write_all(fd, out, out_n);
}

void nova_put_u64(unsigned long value) { nova_fd_put_u64(NOVA_STDOUT, value); }

void nova_put_bytes(unsigned long bytes) {
    const unsigned long mib = 1024UL * 1024UL;
    const unsigned long gib = 1024UL * mib;
    if (bytes >= gib) {
        nova_put_u64(bytes / gib);
        nova_puts(" GiB");
    } else if (bytes >= mib) {
        nova_put_u64(bytes / mib);
        nova_puts(" MiB");
    } else {
        nova_put_u64(bytes);
        nova_puts(" B");
    }
}

long nova_readline(char *buf, nova_size_t capacity) {
    if (!buf || capacity < 2) return -1;
    nova_size_t len = 0;
    for (;;) {
        char c = 0;
        long n = nova_read(NOVA_STDIN, &c, 1);
        if (n < 0) return n;
        if (n == 0) {
            if (len == 0) return -2;
            break;
        }
        if (c == '\n' || c == '\r') break;
        if (len + 1 < capacity) buf[len++] = c;
    }
    buf[len] = '\0';
    return (long)len;
}

void nova_zero(void *ptr, nova_size_t n) {
    unsigned char *p = (unsigned char *)ptr;
    while (n--) *p++ = 0;
}

void nova_copy(void *dst, const void *src, nova_size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
}
