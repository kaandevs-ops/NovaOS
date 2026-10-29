#ifndef NOVA_DESKTOP_PROTOCOL_H
#define NOVA_DESKTOP_PROTOCOL_H

#define NOVA_DESKTOP_SOCKET_PATH "/run/nova/desktop.sock"
#define NOVA_DESKTOP_PROTOCOL_MAGIC 0x4e44534bU
#define NOVA_DESKTOP_PROTOCOL_VERSION 1U
#define NOVA_DESKTOP_TASK_NAME_BYTES 64U
#define NOVA_DESKTOP_TASK_MAX_ENTRIES 64U

enum {
    NOVA_DESKTOP_OP_HELLO = 1U,
    NOVA_DESKTOP_OP_HEARTBEAT = 2U,
    NOVA_DESKTOP_OP_TASK_EVENT = 3U,
    NOVA_DESKTOP_OP_ACTIVATE = 4U,
    NOVA_DESKTOP_OP_CLOSE = 5U,
    NOVA_DESKTOP_OP_LAUNCH = 6U,

    /* Phase-14 additive task-model frames. The original 48-byte control ABI
     * and protocol version remain unchanged. */
    NOVA_DESKTOP_OP_TASK_MODEL_RESET = 7U,
    NOVA_DESKTOP_OP_TASK_MODEL_DEFINE = 8U,
    NOVA_DESKTOP_OP_TASK_MODEL_COMMIT = 9U,
    NOVA_DESKTOP_OP_TASK_MODEL_ICON = 10U,
};

enum {
    NOVA_DESKTOP_TASK_FLAG_LAUNCHABLE = 1U << 0,
    NOVA_DESKTOP_TASK_FLAG_HAS_ICON = 1U << 1,
};

#define NOVA_DESKTOP_TASK_ICON_FORMAT_A8 1U
#define NOVA_DESKTOP_TASK_ICON_WIDTH 24U
#define NOVA_DESKTOP_TASK_ICON_HEIGHT 24U
#define NOVA_DESKTOP_TASK_ICON_BYTES 576U
#define NOVA_DESKTOP_TASK_ICON_DIGEST_BYTES 32U
#define NOVA_DESKTOP_TASK_ICON_ASSET_ID_BYTES 48U

enum {
    NOVA_DESKTOP_ACTION_NONE = 0U,
    NOVA_DESKTOP_ACTION_ACTIVATE = 1U,
    NOVA_DESKTOP_ACTION_CLOSE = 2U,
    NOVA_DESKTOP_ACTION_LAUNCH = 3U,
};

struct nova_desktop_message {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;
    unsigned int client_id;
    unsigned int surface_id;
    unsigned int action;
    unsigned int sequence;
    int cursor_x;
    int cursor_y;
    unsigned int flags;
    unsigned int reserved;
};

_Static_assert(sizeof(struct nova_desktop_message) == 48, "Nova desktop control ABI size mismatch");

/*
 * Phase-14 shell-owned task model. RESET/DEFINE/COMMIT form an atomic bounded
 * transaction on the existing SOCK_SEQPACKET desktop-control connection.
 * display_name is presentation-only; authorization remains in nova-shell/novad.
 */
struct nova_desktop_task_message {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int sequence;
    unsigned int index;
    unsigned int total_count;
    unsigned int client_id;

    unsigned int task_flags;
    unsigned int reserved;
    unsigned long long catalog_generation;

    char display_name[NOVA_DESKTOP_TASK_NAME_BYTES];
};

_Static_assert(sizeof(struct nova_desktop_task_message) == 112, "Nova desktop task-model ABI size mismatch");

/*
 * Phase-15 additive rich task icon frame. It binds a verified immutable icon
 * payload to one already-defined staging task. The original 48-byte control
 * ABI and 112-byte task-definition ABI remain unchanged.
 */
struct nova_desktop_task_icon_message {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int sequence;
    unsigned int index;
    unsigned int total_count;
    unsigned int client_id;

    unsigned int icon_format;
    unsigned int icon_width;
    unsigned int icon_height;
    unsigned int icon_bytes;

    unsigned long long catalog_generation;

    unsigned char digest[NOVA_DESKTOP_TASK_ICON_DIGEST_BYTES];
    char asset_id[NOVA_DESKTOP_TASK_ICON_ASSET_ID_BYTES];
    unsigned char pixels[NOVA_DESKTOP_TASK_ICON_BYTES];
};

_Static_assert(sizeof(struct nova_desktop_task_icon_message) == 712,
               "Nova desktop rich task-icon ABI size mismatch");

#endif
