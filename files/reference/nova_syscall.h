#ifndef NOVA_SYSCALL_H
#define NOVA_SYSCALL_H

#include "nova_linux.h"

#if defined(__x86_64__)
static inline long nova_syscall0(long n) {
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n) : "rcx", "r11", "memory");
    return r;
}
static inline long nova_syscall1(long n, long a1) {
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a1) : "rcx", "r11", "memory");
    return r;
}
static inline long nova_syscall2(long n, long a1, long a2) {
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
    return r;
}
static inline long nova_syscall3(long n, long a1, long a2, long a3) {
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    return r;
}
static inline long nova_syscall4(long n, long a1, long a2, long a3, long a4) {
    register long r10 __asm__("r10") = a4;
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10) : "rcx", "r11", "memory");
    return r;
}
static inline long nova_syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
    register long r10 __asm__("r10") = a4;
    register long r8 __asm__("r8") = a5;
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8) : "rcx", "r11", "memory");
    return r;
}
static inline long nova_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    register long r10 __asm__("r10") = a4;
    register long r8 __asm__("r8") = a5;
    register long r9 __asm__("r9") = a6;
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
    return r;
}
#elif defined(__aarch64__)
static inline long nova_syscall0(long n) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0");
    __asm__ volatile ("svc #0" : "=r"(x0) : "r"(x8) : "memory", "cc");
    return x0;
}
static inline long nova_syscall1(long n, long a1) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a1;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x8) : "memory", "cc");
    return x0;
}
static inline long nova_syscall2(long n, long a1, long a2) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a1;
    register long x1 __asm__("x1") = a2;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x8) : "memory", "cc");
    return x0;
}
static inline long nova_syscall3(long n, long a1, long a2, long a3) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a1;
    register long x1 __asm__("x1") = a2;
    register long x2 __asm__("x2") = a3;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory", "cc");
    return x0;
}
static inline long nova_syscall4(long n, long a1, long a2, long a3, long a4) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a1;
    register long x1 __asm__("x1") = a2;
    register long x2 __asm__("x2") = a3;
    register long x3 __asm__("x3") = a4;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x8) : "memory", "cc");
    return x0;
}
static inline long nova_syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a1;
    register long x1 __asm__("x1") = a2;
    register long x2 __asm__("x2") = a3;
    register long x3 __asm__("x3") = a4;
    register long x4 __asm__("x4") = a5;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x8) : "memory", "cc");
    return x0;
}
static inline long nova_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a1;
    register long x1 __asm__("x1") = a2;
    register long x2 __asm__("x2") = a3;
    register long x3 __asm__("x3") = a4;
    register long x4 __asm__("x4") = a5;
    register long x5 __asm__("x5") = a6;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8) : "memory", "cc");
    return x0;
}
#endif

static inline long nova_read(int fd, void *buf, nova_size_t count) {
    return nova_syscall3(NOVA_NR_read, fd, (long)buf, (long)count);
}
static inline long nova_write(int fd, const void *buf, nova_size_t count) {
    return nova_syscall3(NOVA_NR_write, fd, (long)buf, (long)count);
}
static inline long nova_open_ro(const char *path) {
    return nova_syscall4(NOVA_NR_openat, NOVA_AT_FDCWD, (long)path, NOVA_O_RDONLY, 0);
}
static inline long nova_open_ro_nonblock(const char *path) {
    return nova_syscall4(NOVA_NR_openat, NOVA_AT_FDCWD, (long)path, NOVA_O_RDONLY | NOVA_O_NONBLOCK, 0);
}
static inline long nova_ioctl(int fd, unsigned long request, void *arg) {
    return nova_syscall3(NOVA_NR_ioctl, fd, (long)request, (long)arg);
}
static inline void *nova_mmap(void *addr, nova_size_t length, int prot, int flags, int fd, nova_off_t offset) {
    long r = nova_syscall6(NOVA_NR_mmap, (long)addr, (long)length, prot, flags, fd, (long)offset);
    return (void *)r;
}
static inline long nova_munmap(void *addr, nova_size_t length) {
    return nova_syscall2(NOVA_NR_munmap, (long)addr, (long)length);
}
static inline long nova_poll(struct nova_pollfd *fds, unsigned long nfds, const struct nova_timespec *timeout) {
    return nova_syscall5(NOVA_NR_ppoll, (long)fds, (long)nfds, (long)timeout, 0, 0);
}
static inline long nova_socket(int domain, int type, int protocol) {
    return nova_syscall3(NOVA_NR_socket, domain, type, protocol);
}
static inline long nova_getsockopt(int fd, int level, int optname, void *optval, unsigned int *optlen) {
    return nova_syscall5(NOVA_NR_getsockopt, fd, level, optname, (long)optval, (long)optlen);
}
static inline long nova_bind(int fd, const struct nova_sockaddr_un *addr, unsigned long addrlen) {
    return nova_syscall3(NOVA_NR_bind, fd, (long)addr, (long)addrlen);
}
static inline long nova_listen(int fd, int backlog) {
    return nova_syscall2(NOVA_NR_listen, fd, backlog);
}
static inline long nova_accept4(int fd, int flags) {
    return nova_syscall4(NOVA_NR_accept4, fd, 0, 0, flags);
}
static inline long nova_connect(int fd, const struct nova_sockaddr_un *addr, unsigned long addrlen) {
    return nova_syscall3(NOVA_NR_connect, fd, (long)addr, (long)addrlen);
}
static inline long nova_sendmsg(int fd, const struct nova_msghdr *msg, int flags) {
    return nova_syscall3(NOVA_NR_sendmsg, fd, (long)msg, flags);
}
static inline long nova_recvmsg(int fd, struct nova_msghdr *msg, int flags) {
    return nova_syscall3(NOVA_NR_recvmsg, fd, (long)msg, flags);
}
static inline long nova_memfd_create(const char *name, unsigned int flags) {
    return nova_syscall2(NOVA_NR_memfd_create, (long)name, flags);
}
static inline long nova_ftruncate(int fd, nova_off_t length) {
    return nova_syscall2(NOVA_NR_ftruncate, fd, (long)length);
}
static inline long nova_lseek(int fd, nova_off_t offset, int whence) {
    return nova_syscall3(NOVA_NR_lseek, fd, (long)offset, whence);
}
static inline unsigned long nova_makedev(unsigned long major, unsigned long minor) {
    return ((major & 0xfffff000UL) << 32) | ((major & 0xfffUL) << 8) |
           ((minor & 0xffffff00UL) << 12) | (minor & 0xffUL);
}
static inline long nova_mknod(const char *path, unsigned long mode, unsigned long major, unsigned long minor) {
    return nova_syscall4(NOVA_NR_mknodat, NOVA_AT_FDCWD, (long)path, (long)mode,
                         (long)nova_makedev(major, minor));
}
static inline long nova_open_wo(const char *path) {
    return nova_syscall4(NOVA_NR_openat, NOVA_AT_FDCWD, (long)path, NOVA_O_WRONLY, 0);
}
static inline long nova_open_rw(const char *path) {
    return nova_syscall4(NOVA_NR_openat, NOVA_AT_FDCWD, (long)path, NOVA_O_RDWR, 0);
}
static inline long nova_open_append(const char *path, unsigned long mode) {
    return nova_syscall4(NOVA_NR_openat, NOVA_AT_FDCWD, (long)path,
                         NOVA_O_WRONLY | NOVA_O_CREAT | NOVA_O_APPEND, (long)mode);
}
static inline long nova_open_write(const char *path, unsigned long mode) {
    return nova_syscall4(NOVA_NR_openat, NOVA_AT_FDCWD, (long)path,
                         NOVA_O_WRONLY | NOVA_O_CREAT | NOVA_O_TRUNC, (long)mode);
}
static inline long nova_open_create_excl(const char *path, unsigned long mode) {
    return nova_syscall4(NOVA_NR_openat, NOVA_AT_FDCWD, (long)path,
                         NOVA_O_WRONLY | NOVA_O_CREAT | NOVA_O_EXCL, (long)mode);
}
static inline long nova_close(int fd) {
    return nova_syscall1(NOVA_NR_close, fd);
}
static inline long nova_mount(const char *src, const char *target, const char *fstype, unsigned long flags, const void *data) {
    return nova_syscall5(NOVA_NR_mount, (long)src, (long)target, (long)fstype, (long)flags, (long)data);
}
static inline long nova_unshare(unsigned long flags) {
    return nova_syscall1(NOVA_NR_unshare, (long)flags);
}
static inline long nova_prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) {
    return nova_syscall5(NOVA_NR_prctl, option, (long)arg2, (long)arg3, (long)arg4, (long)arg5);
}
static inline long nova_umount2(const char *target, int flags) {
    return nova_syscall2(NOVA_NR_umount2, (long)target, (long)flags);
}
static inline long nova_mkdir(const char *path, unsigned long mode) {
    return nova_syscall3(NOVA_NR_mkdirat, NOVA_AT_FDCWD, (long)path, (long)mode);
}
static inline long nova_fchownat(const char *path, unsigned int uid, unsigned int gid) {
    return nova_syscall5(NOVA_NR_fchownat, NOVA_AT_FDCWD, (long)path, (long)uid, (long)gid, 0);
}
static inline long nova_fchmodat(const char *path, unsigned long mode) {
    return nova_syscall3(NOVA_NR_fchmodat, NOVA_AT_FDCWD, (long)path, (long)mode);
}
static inline long nova_chroot(const char *path) {
    return nova_syscall1(NOVA_NR_chroot, (long)path);
}
static inline long nova_chdir(const char *path) {
    return nova_syscall1(NOVA_NR_chdir, (long)path);
}
static inline long nova_setgroups(unsigned long count, const unsigned int *groups) {
    return nova_syscall2(NOVA_NR_setgroups, (long)count, (long)groups);
}
static inline long nova_setgid(unsigned int gid) {
    return nova_syscall1(NOVA_NR_setgid, (long)gid);
}
static inline long nova_setuid(unsigned int uid) {
    return nova_syscall1(NOVA_NR_setuid, (long)uid);
}
static inline long nova_close_range(unsigned int first, unsigned int last, unsigned int flags) {
    return nova_syscall3(NOVA_NR_close_range, (long)first, (long)last, (long)flags);
}
static inline long nova_unlink(const char *path) {
    return nova_syscall3(NOVA_NR_unlinkat, NOVA_AT_FDCWD, (long)path, 0);
}
static inline long nova_rename(const char *oldpath, const char *newpath) {
    return nova_syscall4(NOVA_NR_renameat, NOVA_AT_FDCWD, (long)oldpath, NOVA_AT_FDCWD, (long)newpath);
}
static inline long nova_fsync(int fd) {
    return nova_syscall1(NOVA_NR_fsync, fd);
}
static inline long nova_uname(struct nova_utsname *u) {
    return nova_syscall1(NOVA_NR_uname, (long)u);
}
static inline long nova_sysinfo(struct nova_sysinfo *i) {
    return nova_syscall1(NOVA_NR_sysinfo, (long)i);
}
static inline long nova_getcwd(char *buf, nova_size_t size) {
    return nova_syscall2(NOVA_NR_getcwd, (long)buf, (long)size);
}
static inline long nova_getpid(void) {
    return nova_syscall0(NOVA_NR_getpid);
}
static inline long nova_sethostname(const char *name, nova_size_t len) {
    return nova_syscall2(NOVA_NR_sethostname, (long)name, (long)len);
}
static inline long nova_sync(void) {
    return nova_syscall0(NOVA_NR_sync);
}
static inline long nova_reboot(int command) {
    return nova_syscall4(NOVA_NR_reboot, NOVA_LINUX_REBOOT_MAGIC1, NOVA_LINUX_REBOOT_MAGIC2, command, 0);
}
static inline long nova_clone_process(void) {
    return nova_syscall5(NOVA_NR_clone, NOVA_SIGCHLD, 0, 0, 0, 0);
}
static inline long nova_execve(const char *path, char *const argv[], char *const envp[]) {
    return nova_syscall3(NOVA_NR_execve, (long)path, (long)argv, (long)envp);
}
static inline long nova_wait4(long pid, int *status, int options) {
    return nova_syscall4(NOVA_NR_wait4, pid, (long)status, options, 0);
}
static inline long nova_kill(long pid, int sig) {
    return nova_syscall2(NOVA_NR_kill, pid, sig);
}
static inline long nova_add_key(const char *type, const char *description, const void *payload, nova_size_t plen, long keyring) {
    return nova_syscall5(NOVA_NR_add_key, (long)type, (long)description, (long)payload, (long)plen, keyring);
}
static inline long nova_keyctl_get_keyring_id(long id, int create) {
    return nova_syscall5(NOVA_NR_keyctl, NOVA_KEYCTL_GET_KEYRING_ID, id, create, 0, 0);
}
static inline long nova_keyctl_pkey_verify(struct nova_keyctl_pkey_params *params, const char *info, const void *digest, const void *signature) {
    return nova_syscall5(NOVA_NR_keyctl, NOVA_KEYCTL_PKEY_VERIFY, (long)params, (long)info, (long)digest, (long)signature);
}
static inline long nova_nanosleep(const struct nova_timespec *req, struct nova_timespec *rem) {
    return nova_syscall2(NOVA_NR_nanosleep, (long)req, (long)rem);
}
static inline long nova_clock_gettime(int clock_id, struct nova_timespec *ts) {
    return nova_syscall2(NOVA_NR_clock_gettime, clock_id, (long)ts);
}
static inline __attribute__((noreturn)) void nova_exit(int status) {
    nova_syscall1(NOVA_NR_exit, status);
    for (;;) { }
}

#endif
