/* Read-only diagnostics of the calling terminal session, never global telemetry. */
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

static void capacity(const char *label, unsigned long long blocks,
                     unsigned long long unit) {
    if (unit && blocks > UINT64_MAX / unit)
        printf("%s: unavailable (numeric range)\n", label);
    else
        printf("%s: %" PRIu64 " bytes\n", label, (uint64_t)blocks * (uint64_t)unit);
}
static void limit(const char *name, int resource) {
    struct rlimit value;
    if (getrlimit(resource, &value) != 0) {
        printf("%s: unavailable (%s)\n", name, strerror(errno));
    } else if (value.rlim_cur == RLIM_INFINITY) {
        printf("%s: unlimited\n", name);
    } else {
        printf("%s: %" PRIuMAX "\n", name, (uintmax_t)value.rlim_cur);
    }
}
int main(int argc, char **argv) {
    struct utsname kernel;
    struct stat st;
    struct statvfs fs;
    char cwd[4096];
    const char *path = argc == 2 ? argv[1] : "/home/nova";
    int failed = 0;
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("Usage: nova-info [PATH]\nRead-only session identity, kernel, resource limits and filesystem capacity.\nPATH defaults to /home/nova. Values describe this confined session, not host RAM or whole-system telemetry.");
        return 0;
    }
    if (argc > 2 || (argc == 2 && argv[1][0] == '-')) {
        fputs("Usage: nova-info [PATH]\n", stderr);
        return 2;
    }
    puts("Nova Terminal - measured session diagnostics");
    printf("Identity: uid=%lu euid=%lu gid=%lu egid=%lu\n",
           (unsigned long)getuid(), (unsigned long)geteuid(),
           (unsigned long)getgid(), (unsigned long)getegid());
    if (uname(&kernel) == 0)
        printf("Kernel: %s %s (%s)\n", kernel.sysname, kernel.release, kernel.machine);
    else { printf("Kernel: unavailable (%s)\n", strerror(errno)); failed = 1; }
    if (getcwd(cwd, sizeof(cwd))) printf("Directory: %s\n", cwd);
    else { printf("Directory: unavailable (%s)\n", strerror(errno)); failed = 1; }
    limit("Session FD limit", RLIMIT_NOFILE);
    limit("Session address-space limit (bytes)", RLIMIT_AS);
    limit("Session process limit", RLIMIT_NPROC);
    printf("Filesystem path: %s\n", path);
    if (stat(path, &st) != 0) {
        printf("Path: unavailable (%s)\n", strerror(errno));
        failed = 1;
    } else {
        printf("Path owner: %lu:%lu; mode: %03o; type: %s\n",
               (unsigned long)st.st_uid, (unsigned long)st.st_gid,
               (unsigned int)(st.st_mode & 0777),
               S_ISDIR(st.st_mode) ? "directory" : S_ISREG(st.st_mode) ? "regular file" : "other");
    }
    if (statvfs(path, &fs) != 0) {
        printf("Filesystem capacity: unavailable (%s)\n", strerror(errno));
        failed = 1;
    } else {
        unsigned long long unit = fs.f_frsize ? fs.f_frsize : fs.f_bsize;
        capacity("Filesystem total", fs.f_blocks, unit);
        capacity("Filesystem available to caller", fs.f_bavail, unit);
        printf("Filesystem mount: %s\n", (fs.f_flag & ST_RDONLY) ? "read-only" : "writable (DAC still applies)");
    }
    puts("Scope: confined session. Filesystem capacity is not system RAM.\nWritable does not establish reboot persistence; verify a reboot proof separately.\nApplications: nova-pkg list shows the verified LOCAL catalog; general repositories and GUI browser compatibility remain pending.\nHelp: busybox --help and COMMAND --help describe supported command options.");
    return ferror(stdout) || failed ? 1 : 0;
}