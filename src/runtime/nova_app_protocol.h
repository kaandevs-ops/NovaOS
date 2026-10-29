#ifndef NOVA_APP_PROTOCOL_H
#define NOVA_APP_PROTOCOL_H

/*
 * NovaOS application lifecycle IPC.
 *
 * Security boundary: clients identify an application only by a bounded,
 * NUL-terminated application ID. No executable path, argv, environment, or
 * shell command is transported over this protocol; novad resolves app IDs
 * through its integrity-protected application registry.
 */

#define NOVA_APP_SOCKET_PATH "/run/nova/app-lifecycle.sock"
#define NOVA_APP_PROTOCOL_MAGIC 0x4e415050U
#define NOVA_APP_PROTOCOL_VERSION 1U
#define NOVA_APP_ID_BYTES 32U

enum {
    NOVA_APP_OP_HELLO = 1U,
    NOVA_APP_OP_LAUNCH = 2U,
    NOVA_APP_OP_STOP = 3U,
    NOVA_APP_OP_QUERY = 4U,
    NOVA_APP_OP_STATUS = 0x100U,
};

enum {
    NOVA_APP_STATE_UNKNOWN = 0U,
    NOVA_APP_STATE_STOPPED = 1U,
    NOVA_APP_STATE_STARTING = 2U,
    NOVA_APP_STATE_RUNNING = 3U,
    NOVA_APP_STATE_STOPPING = 4U,
    NOVA_APP_STATE_RESTARTING = 5U,
    NOVA_APP_STATE_FAILED = 6U,
};

enum {
    NOVA_APP_RESULT_OK = 0U,
    NOVA_APP_RESULT_UNKNOWN_APP = 1U,
    NOVA_APP_RESULT_ALREADY_RUNNING = 2U,
    NOVA_APP_RESULT_NOT_RUNNING = 3U,
    NOVA_APP_RESULT_SPAWN_FAILED = 4U,
    NOVA_APP_RESULT_BAD_REQUEST = 5U,
    NOVA_APP_RESULT_DENIED = 6U,
    NOVA_APP_RESULT_STOP_FAILED = 7U,
};

struct nova_app_message {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int request_id;
    unsigned int flags;

    unsigned long long generation;
    long long pid;

    unsigned int state;
    unsigned int result;

    char app_id[NOVA_APP_ID_BYTES];
};

_Static_assert(
    sizeof(struct nova_app_message) == 80,
    "Nova application lifecycle ABI size mismatch"
);

#endif
