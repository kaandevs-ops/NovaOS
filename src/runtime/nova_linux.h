#ifndef NOVA_LINUX_H
#define NOVA_LINUX_H

/* Minimal Linux UAPI subset used by NovaOS native userspace.
 * Intentionally avoids libc and external Linux headers. */

typedef unsigned long nova_size_t;
typedef long nova_ssize_t;
typedef long nova_off_t;

enum {
    NOVA_STDIN  = 0,
    NOVA_STDOUT = 1,
    NOVA_STDERR = 2,
    NOVA_EINTR = 4,
    NOVA_EAGAIN = 11,
    NOVA_EINPROGRESS = 115,
    NOVA_AT_FDCWD = -100,
    NOVA_AT_EMPTY_PATH = 0x1000,

    NOVA_AF_UNIX = 1,
    NOVA_AF_INET = 2,
    NOVA_SOCK_STREAM = 1,
    NOVA_SOCK_SEQPACKET = 5,
    NOVA_SOCK_NONBLOCK = 2048,

    NOVA_SOL_SOCKET = 1,
    NOVA_SO_PEERCRED = 17,
    NOVA_SCM_RIGHTS = 1,
    NOVA_MSG_CTRUNC = 0x08,
    NOVA_MSG_TRUNC = 0x20,
    NOVA_MSG_DONTWAIT = 0x40,
    NOVA_MSG_CMSG_CLOEXEC = 0x40000000,
    NOVA_MSG_NOSIGNAL = 0x4000,

    NOVA_MFD_CLOEXEC = 0x0001,

    NOVA_O_RDONLY = 0,
    NOVA_O_WRONLY = 1,
    NOVA_O_RDWR   = 2,
    NOVA_O_CREAT  = 64,
    NOVA_O_EXCL   = 128,
    NOVA_O_TRUNC  = 512,
    NOVA_O_APPEND = 1024,
    NOVA_O_NONBLOCK = 2048,
    NOVA_O_CLOEXEC = 02000000,
    NOVA_O_NOFOLLOW = 00400000,

    NOVA_PROT_READ  = 1,
    NOVA_PROT_WRITE = 2,
    NOVA_MAP_SHARED = 1,
    NOVA_MAP_PRIVATE = 0x02,
    NOVA_MAP_ANONYMOUS = 0x20,

    NOVA_SEEK_SET = 0,
    NOVA_SEEK_END = 2,

    NOVA_POLLIN  = 0x001,
    NOVA_POLLERR = 0x008,
    NOVA_POLLHUP = 0x010,

    NOVA_S_IFMT = 0170000,
    NOVA_S_IFREG = 0100000,
    NOVA_S_IFCHR = 0020000,
    NOVA_S_IFBLK = 0060000,

    NOVA_MS_RDONLY = 1,
    NOVA_MS_NOSUID = 2,
    NOVA_MS_NODEV = 4,
    NOVA_MS_NOEXEC = 8,
    NOVA_MS_REMOUNT = 32,
    NOVA_MS_BIND = 4096,
    NOVA_MS_REC = 16384,
    NOVA_MS_PRIVATE = 262144,

    NOVA_CLONE_NEWNS = 0x00020000,
    NOVA_PR_SET_SECCOMP = 22,
    NOVA_PR_SET_NO_NEW_PRIVS = 38,
    NOVA_SECCOMP_MODE_FILTER = 2,
    NOVA_EPERM = 1,
    NOVA_CLOSE_RANGE_CLOEXEC = 1U << 2,

    NOVA_WNOHANG = 1,
    NOVA_CLOCK_MONOTONIC = 1,
    NOVA_SIGTERM = 15,
    NOVA_SIGCHLD = 17,
    NOVA_SIGKILL = 9,

    NOVA_KEY_SPEC_PROCESS_KEYRING = -2,
    NOVA_KEYCTL_GET_KEYRING_ID = 0,
    NOVA_KEYCTL_PKEY_VERIFY = 28,

    NOVA_DM_IOCTL = 0xfd,
    NOVA_DM_VERSION_CMD = 0,
    NOVA_DM_DEV_CREATE_CMD = 3,
    NOVA_DM_DEV_SUSPEND_CMD = 6,
    NOVA_DM_TABLE_LOAD_CMD = 9,
    NOVA_DM_READONLY_FLAG = 1,

    NOVA_RB_AUTOBOOT = 0x01234567,
    NOVA_RB_POWER_OFF = 0x4321FEDC,
    NOVA_LINUX_REBOOT_MAGIC1 = 0xfee1dead,
    NOVA_LINUX_REBOOT_MAGIC2 = 672274793,
};


struct nova_sock_filter {
    unsigned short code;
    unsigned char jt;
    unsigned char jf;
    unsigned int k;
};

struct nova_sock_fprog {
    unsigned short len;
    struct nova_sock_filter *filter;
};

_Static_assert(sizeof(struct nova_sock_filter) == 8, "classic BPF instruction ABI size mismatch");
_Static_assert(sizeof(struct nova_sock_fprog) == 16, "sock_fprog ABI size mismatch");

#define NOVA_BPF_LD   0x00U
#define NOVA_BPF_W    0x00U
#define NOVA_BPF_ABS  0x20U
#define NOVA_BPF_JMP  0x05U
#define NOVA_BPF_JEQ  0x10U
#define NOVA_BPF_K    0x00U
#define NOVA_BPF_RET  0x06U
#define NOVA_BPF_STMT(code_, k_) { (unsigned short)(code_), 0, 0, (unsigned int)(k_) }
#define NOVA_BPF_JUMP(code_, k_, jt_, jf_) { (unsigned short)(code_), (unsigned char)(jt_), (unsigned char)(jf_), (unsigned int)(k_) }

#define NOVA_SECCOMP_DATA_NR_OFFSET 0U
#define NOVA_SECCOMP_DATA_ARCH_OFFSET 4U
#define NOVA_SECCOMP_DATA_ARG0_OFFSET 16U
#define NOVA_SECCOMP_DATA_ARG1_OFFSET 24U
#define NOVA_SECCOMP_DATA_ARG2_OFFSET 32U

#define NOVA_SECCOMP_RET_KILL_PROCESS 0x80000000U
#define NOVA_SECCOMP_RET_ERRNO        0x00050000U
#define NOVA_SECCOMP_RET_ALLOW        0x7fff0000U
#define NOVA_SECCOMP_RET_DATA         0x0000ffffU

#if defined(__x86_64__)
#define NOVA_AUDIT_ARCH_NATIVE 0xc000003eU
#elif defined(__aarch64__)
#define NOVA_AUDIT_ARCH_NATIVE 0xc00000b7U
#endif

/* Linux statx UAPI for broker grant checks and descriptor-only Files metadata.
 * R22 adds no syscall capability or general filesystem visibility. */
#define NOVA_STATX_TYPE 0x00000001U
#define NOVA_STATX_SIZE 0x00000200U
struct nova_statx_timestamp {
    long long tv_sec;
    unsigned int tv_nsec;
    int __reserved;
};

struct nova_statx {
    unsigned int stx_mask;
    unsigned int stx_blksize;
    unsigned long long stx_attributes;
    unsigned int stx_nlink;
    unsigned int stx_uid;
    unsigned int stx_gid;
    unsigned short stx_mode;
    unsigned short __spare0;
    unsigned long long stx_ino;
    unsigned long long stx_size;
    unsigned long long stx_blocks;
    unsigned long long stx_attributes_mask;
    struct nova_statx_timestamp stx_atime;
    struct nova_statx_timestamp stx_btime;
    struct nova_statx_timestamp stx_ctime;
    struct nova_statx_timestamp stx_mtime;
    unsigned int stx_rdev_major;
    unsigned int stx_rdev_minor;
    unsigned int stx_dev_major;
    unsigned int stx_dev_minor;
    unsigned long long stx_mnt_id;
    unsigned int stx_dio_mem_align;
    unsigned int stx_dio_offset_align;
    unsigned char __spare3[96];
};
_Static_assert(sizeof(struct nova_statx) == 256, "statx UAPI size mismatch");
_Static_assert(__builtin_offsetof(struct nova_statx, stx_mode) == 28, "statx mode offset mismatch");
_Static_assert(__builtin_offsetof(struct nova_statx, stx_size) == 40, "statx size offset mismatch");

struct nova_timespec {
    long tv_sec;
    long tv_nsec;
};

struct nova_pollfd {
    int fd;
    short events;
    short revents;
};


struct nova_iovec {
    void *iov_base;
    nova_size_t iov_len;
};

struct nova_msghdr {
    void *msg_name;
    unsigned int msg_namelen;
    struct nova_iovec *msg_iov;
    nova_size_t msg_iovlen;
    void *msg_control;
    nova_size_t msg_controllen;
    int msg_flags;
};

struct nova_cmsghdr {
    nova_size_t cmsg_len;
    int cmsg_level;
    int cmsg_type;
};

#define NOVA_CMSG_ALIGN(len) (((len) + sizeof(nova_size_t) - 1U) & ~(sizeof(nova_size_t) - 1U))
#define NOVA_CMSG_DATA_OFFSET NOVA_CMSG_ALIGN(sizeof(struct nova_cmsghdr))
#define NOVA_CMSG_LEN(len) (NOVA_CMSG_DATA_OFFSET + (len))
#define NOVA_CMSG_SPACE(len) NOVA_CMSG_ALIGN(NOVA_CMSG_LEN(len))

_Static_assert(sizeof(struct nova_iovec) == 16, "iovec ABI size mismatch");
_Static_assert(sizeof(struct nova_msghdr) == 56, "msghdr ABI size mismatch");
_Static_assert(sizeof(struct nova_cmsghdr) == 16, "cmsghdr ABI size mismatch");
_Static_assert(NOVA_CMSG_SPACE(sizeof(int)) == 24, "SCM_RIGHTS control size mismatch");

struct nova_sockaddr_un {
    unsigned short sun_family;
    char sun_path[108];
};

_Static_assert(sizeof(struct nova_sockaddr_un) == 110, "sockaddr_un ABI size mismatch");

struct nova_ucred {
    int pid;
    unsigned int uid;
    unsigned int gid;
};

_Static_assert(sizeof(struct nova_ucred) == 12, "ucred ABI size mismatch");

struct nova_utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

struct nova_keyctl_pkey_params {
    int key_id;
    unsigned int in_len;
    union {
        unsigned int out_len;
        unsigned int in2_len;
    };
    unsigned int spare[7];
};


struct nova_dm_ioctl {
    unsigned int version[3];
    unsigned int data_size;
    unsigned int data_start;
    unsigned int target_count;
    int open_count;
    unsigned int flags;
    unsigned int event_nr;
    unsigned int padding;
    unsigned long long dev;
    char name[128];
    char uuid[129];
    char data[7];
};

struct nova_dm_target_spec {
    unsigned long long sector_start;
    unsigned long long length;
    int status;
    unsigned int next;
    char target_type[16];
};

_Static_assert(sizeof(struct nova_dm_ioctl) == 312, "device-mapper ioctl ABI size mismatch");
_Static_assert(sizeof(struct nova_dm_target_spec) == 40, "device-mapper target ABI size mismatch");

#define NOVA_DM_IOCTL_ENCODE(nr) ((unsigned long)(3UL << 30) | ((unsigned long)sizeof(struct nova_dm_ioctl) << 16) | ((unsigned long)NOVA_DM_IOCTL << 8) | (unsigned long)(nr))
#define NOVA_DM_VERSION      NOVA_DM_IOCTL_ENCODE(NOVA_DM_VERSION_CMD)
#define NOVA_DM_DEV_CREATE   NOVA_DM_IOCTL_ENCODE(NOVA_DM_DEV_CREATE_CMD)
#define NOVA_DM_DEV_SUSPEND  NOVA_DM_IOCTL_ENCODE(NOVA_DM_DEV_SUSPEND_CMD)
#define NOVA_DM_TABLE_LOAD   NOVA_DM_IOCTL_ENCODE(NOVA_DM_TABLE_LOAD_CMD)

struct nova_sysinfo {
    long uptime;
    unsigned long loads[3];
    unsigned long totalram;
    unsigned long freeram;
    unsigned long sharedram;
    unsigned long bufferram;
    unsigned long totalswap;
    unsigned long freeswap;
    unsigned short procs;
    unsigned short pad;
    unsigned long totalhigh;
    unsigned long freehigh;
    unsigned int mem_unit;
};

#if defined(__x86_64__)
#define NOVA_NR_read          0
#define NOVA_NR_write         1
#define NOVA_NR_close         3
#define NOVA_NR_lseek         8
#define NOVA_NR_mmap          9
#define NOVA_NR_munmap        11
#define NOVA_NR_ioctl         16
#define NOVA_NR_nanosleep     35
#define NOVA_NR_clock_gettime  228
#define NOVA_NR_getpid        39
#define NOVA_NR_socket        41
#define NOVA_NR_getsockopt    55
#define NOVA_NR_connect       42
#define NOVA_NR_sendmsg       46
#define NOVA_NR_recvmsg       47
#define NOVA_NR_bind          49
#define NOVA_NR_listen        50
#define NOVA_NR_accept4       288
#define NOVA_NR_memfd_create  319
#define NOVA_NR_clone         56
#define NOVA_NR_execve        59
#define NOVA_NR_exit          60
#define NOVA_NR_wait4         61
#define NOVA_NR_kill          62
#define NOVA_NR_uname         63
#define NOVA_NR_fsync         74
#define NOVA_NR_ftruncate     77
#define NOVA_NR_getcwd        79
#define NOVA_NR_sysinfo       99
#define NOVA_NR_sync          162
#define NOVA_NR_mount         165
#define NOVA_NR_umount2       166
#define NOVA_NR_reboot        169
#define NOVA_NR_sethostname   170
#define NOVA_NR_prctl         157
#define NOVA_NR_unshare       272
#define NOVA_NR_openat        257
#define NOVA_NR_statx         332
#define NOVA_NR_ppoll         271
#define NOVA_NR_mknodat       259
#define NOVA_NR_mkdirat       258
#define NOVA_NR_unlinkat      263
#define NOVA_NR_renameat      264
#define NOVA_NR_add_key       248
#define NOVA_NR_keyctl        250
#define NOVA_NR_chdir         80
#define NOVA_NR_chroot        161
#define NOVA_NR_setuid        105
#define NOVA_NR_setgid        106
#define NOVA_NR_setgroups     116
#define NOVA_NR_fchownat      260
#define NOVA_NR_fchmodat      268
#define NOVA_NR_close_range   436
#elif defined(__aarch64__)
#define NOVA_NR_getcwd        17
#define NOVA_NR_munmap        215
#define NOVA_NR_lseek         62
#define NOVA_NR_mmap          222
#define NOVA_NR_mknodat       33
#define NOVA_NR_renameat      38
#define NOVA_NR_mkdirat       34
#define NOVA_NR_unlinkat      35
#define NOVA_NR_umount2       39
#define NOVA_NR_mount         40
#define NOVA_NR_ioctl         29
#define NOVA_NR_ppoll         73
#define NOVA_NR_openat        56
#define NOVA_NR_statx         291
#define NOVA_NR_close         57
#define NOVA_NR_ftruncate     46
#define NOVA_NR_read          63
#define NOVA_NR_write         64
#define NOVA_NR_sync          81
#define NOVA_NR_fsync         82
#define NOVA_NR_nanosleep     101
#define NOVA_NR_clock_gettime  113
#define NOVA_NR_socket        198
#define NOVA_NR_getsockopt    209
#define NOVA_NR_sendmsg       211
#define NOVA_NR_recvmsg       212
#define NOVA_NR_bind          200
#define NOVA_NR_listen        201
#define NOVA_NR_connect       203
#define NOVA_NR_accept4       242
#define NOVA_NR_memfd_create  279
#define NOVA_NR_kill          129
#define NOVA_NR_reboot        142
#define NOVA_NR_uname         160
#define NOVA_NR_sethostname   161
#define NOVA_NR_prctl         167
#define NOVA_NR_unshare       97
#define NOVA_NR_getpid        172
#define NOVA_NR_sysinfo       179
#define NOVA_NR_clone         220
#define NOVA_NR_execve        221
#define NOVA_NR_exit          93
#define NOVA_NR_wait4         260
#define NOVA_NR_add_key       217
#define NOVA_NR_keyctl        219
#define NOVA_NR_chdir         49
#define NOVA_NR_chroot        51
#define NOVA_NR_setuid        146
#define NOVA_NR_setgid        144
#define NOVA_NR_setgroups     159
#define NOVA_NR_fchownat      54
#define NOVA_NR_fchmodat      53
#define NOVA_NR_close_range   436
#else
#error "NovaOS native userspace supports only x86_64 and aarch64"
#endif

#endif
