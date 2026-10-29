#ifndef NOVA_APP_SANDBOX_H
#define NOVA_APP_SANDBOX_H

/* Stage-5A Phase-17 application sandbox policy foundation.
 *
 * Manifest authors choose only a bounded named profile.  The profile expands
 * inside trusted novad code to kernel-enforced requirements.  Applications
 * never provide mount paths, namespace flags, seccomp bytecode, syscall
 * numbers, or other low-level sandbox authority through their manifest.
 */

#define NOVA_APP_SANDBOX_PROFILE_DESKTOP_V1_NAME "desktop-v1"

enum nova_app_sandbox_profile {
    NOVA_APP_SANDBOX_PROFILE_NONE = 0,
    NOVA_APP_SANDBOX_PROFILE_DESKTOP_V1 = 1,
};

enum nova_app_sandbox_flags {
    NOVA_APP_SANDBOX_FLAG_NO_NEW_PRIVS     = 1ULL << 0,
    NOVA_APP_SANDBOX_FLAG_MOUNT_NAMESPACE  = 1ULL << 1,
    NOVA_APP_SANDBOX_FLAG_PRIVATE_MOUNTS    = 1ULL << 2,
    NOVA_APP_SANDBOX_FLAG_PRIVATE_ROOT      = 1ULL << 3,
    NOVA_APP_SANDBOX_FLAG_PRIVATE_TMP       = 1ULL << 4,
    NOVA_APP_SANDBOX_FLAG_APP_DATA          = 1ULL << 5,
    NOVA_APP_SANDBOX_FLAG_DROP_PRIVILEGES   = 1ULL << 6,
    NOVA_APP_SANDBOX_FLAG_CLOSE_INHERITED_FDS = 1ULL << 7,
    NOVA_APP_SANDBOX_FLAG_PRIVATE_RUNTIME_IPC  = 1ULL << 8,
    NOVA_APP_SANDBOX_FLAG_SECCOMP_DESKTOP_V1  = 1ULL << 9,
};

#define NOVA_APP_SANDBOX_KNOWN_FLAGS \
    (NOVA_APP_SANDBOX_FLAG_NO_NEW_PRIVS | \
     NOVA_APP_SANDBOX_FLAG_MOUNT_NAMESPACE | \
     NOVA_APP_SANDBOX_FLAG_PRIVATE_MOUNTS | \
     NOVA_APP_SANDBOX_FLAG_PRIVATE_ROOT | \
     NOVA_APP_SANDBOX_FLAG_PRIVATE_TMP | \
     NOVA_APP_SANDBOX_FLAG_APP_DATA | \
     NOVA_APP_SANDBOX_FLAG_DROP_PRIVILEGES | \
     NOVA_APP_SANDBOX_FLAG_CLOSE_INHERITED_FDS | \
     NOVA_APP_SANDBOX_FLAG_PRIVATE_RUNTIME_IPC | \
     NOVA_APP_SANDBOX_FLAG_SECCOMP_DESKTOP_V1)

#define NOVA_APP_SANDBOX_DESKTOP_V1_FLAGS NOVA_APP_SANDBOX_KNOWN_FLAGS

/* Trusted novad-owned filesystem layout.  None of these paths are supplied by
 * application manifests.  app_id is already restricted to a metadata token. */
#define NOVA_APP_SANDBOX_ROOT "/.nova-sandbox-root"
#define NOVA_APP_DATA_ROOT "/Data/Nova/Apps"
#define NOVA_APP_SANDBOX_DATA_PATH "/Data/App"
#define NOVA_APP_SANDBOX_TMP_PATH "/tmp"
#define NOVA_APP_SANDBOX_RUNTIME_DIR "/run/nova"
#define NOVA_APP_SANDBOX_COMPOSITOR_SOCKET "/run/nova/client.sock"

#define NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1_NAME "files-private-read-v1"
#define NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1 2U
/* Same isolation requirements; a different trusted syscall policy is selected. */
#define NOVA_APP_SANDBOX_FILES_READ_V1_FLAGS NOVA_APP_SANDBOX_DESKTOP_V1_FLAGS
/* Dedicated fixed trusted bootstrap. Generic desktop-v1 remains unchanged.
 * The signed helper completes private runtime/PID isolation then permanently
 * drops UID before connecting; no application-supplied path or command. */
#define NOVA_APP_SANDBOX_PROFILE_BROWSER_V1_NAME "browser-compat-v1"
#define NOVA_APP_SANDBOX_PROFILE_BROWSER_V1 3U
#define NOVA_APP_SANDBOX_BROWSER_V1_FLAGS (NOVA_APP_SANDBOX_FLAG_NO_NEW_PRIVS | NOVA_APP_SANDBOX_FLAG_MOUNT_NAMESPACE | NOVA_APP_SANDBOX_FLAG_PRIVATE_MOUNTS | NOVA_APP_SANDBOX_FLAG_CLOSE_INHERITED_FDS)
#endif
