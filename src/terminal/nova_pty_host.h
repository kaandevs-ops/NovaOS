#ifndef NOVA_PTY_HOST_H
#define NOVA_PTY_HOST_H
#include <sys/types.h>
/* POSIX host integration prototype. Not installed in guest or an authority API.
 * Caller supplies trusted absolute executable/argv; never a shell command string.
 * Guest adoption requires a dedicated broker, principal and sandbox policy. */
struct nova_pty { int master; pid_t child; };
int nova_pty_start(struct nova_pty *, const char *, char *const [], unsigned, unsigned);
int nova_pty_resize(struct nova_pty *, unsigned, unsigned);
int nova_pty_close(struct nova_pty *, int *);
#endif
