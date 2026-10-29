#ifndef NOVA_RUNTIME_H
#define NOVA_RUNTIME_H

#include "nova_syscall.h"

nova_size_t nova_strlen(const char *s);
int nova_streq(const char *a, const char *b);
int nova_startswith(const char *s, const char *prefix);
const char *nova_skip_spaces(const char *s);
void nova_puts(const char *s);
void nova_putln(const char *s);
void nova_put_u64(unsigned long value);
void nova_put_bytes(unsigned long bytes);
int nova_fd_write_all(int fd, const void *buf, nova_size_t len);
void nova_fd_puts(int fd, const char *s);
void nova_fd_putln(int fd, const char *s);
void nova_fd_put_u64(int fd, unsigned long value);
long nova_readline(char *buf, nova_size_t capacity);
void nova_zero(void *ptr, nova_size_t n);
void nova_copy(void *dst, const void *src, nova_size_t n);

#endif
