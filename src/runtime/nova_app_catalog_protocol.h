#ifndef NOVA_APP_CATALOG_PROTOCOL_H
#define NOVA_APP_CATALOG_PROTOCOL_H

/*
 * NovaOS application catalog IPC.
 *
 * This is a read-only discovery plane. It is deliberately separate from the
 * Phase-13 application lifecycle control protocol: catalog clients discover
 * bounded, presentation-safe application metadata, while launch/stop/query
 * authority remains on nova_app_protocol.h and is resolved by novad.
 *
 * Security boundary:
 *  - app_id is the only application identity carried across both planes.
 *  - display_name is presentation-only and MUST NOT be used for authorization.
 *  - executable paths, argv, environment strings, and shell commands are never
 *    transported by this protocol.
 *  - novad remains the authority that maps app_id to an integrity-protected
 *    application registry entry.
 */

#include "nova_app_protocol.h"

#define NOVA_APP_CATALOG_SOCKET_PATH "/run/nova/app-catalog.sock"
#define NOVA_APP_CATALOG_PROTOCOL_MAGIC 0x4e434154U /* NCAT */
#define NOVA_APP_CATALOG_PROTOCOL_VERSION 1U
#define NOVA_APP_DISPLAY_NAME_BYTES 64U
#define NOVA_APP_CATALOG_MAX_ENTRIES 64U

enum {
    NOVA_APP_CATALOG_OP_HELLO = 1U,
    NOVA_APP_CATALOG_OP_GET_ENTRY = 2U,

    NOVA_APP_CATALOG_OP_HELLO_REPLY = 0x100U,
    NOVA_APP_CATALOG_OP_ENTRY = 0x101U,
    NOVA_APP_CATALOG_OP_END = 0x102U,
    NOVA_APP_CATALOG_OP_ERROR = 0x103U,
};

enum {
    NOVA_APP_CATALOG_RESULT_OK = 0U,
    NOVA_APP_CATALOG_RESULT_BAD_REQUEST = 1U,
    NOVA_APP_CATALOG_RESULT_DENIED = 2U,
    NOVA_APP_CATALOG_RESULT_INTERNAL = 3U,
};

enum {
    NOVA_APP_CATALOG_FLAG_VISIBLE = 1U << 0,
    NOVA_APP_CATALOG_FLAG_LAUNCHABLE = 1U << 1,
};

/*
 * HELLO requires index=0, flags=0, reserved=0.
 * GET_ENTRY uses a zero-based index, flags=0, reserved=0.
 */
struct nova_app_catalog_request {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int request_id;
    unsigned int index;
    unsigned int flags;
    unsigned int reserved;
};

/*
 * HELLO_REPLY: total_count + catalog_generation are valid; identity fields are
 * empty/zero.
 * ENTRY:       index identifies one catalog entry and identity/state fields are
 *              valid.
 * END:         no entry exists at the requested index; identity fields are
 *              empty/zero. This is the normal bounded iteration terminator.
 * ERROR:       result explains rejection; identity fields are empty/zero.
 *
 * display_name is bounded NUL-terminated presentation text. It is not an
 * identity and must never be interpreted as a filesystem path or command.
 */
struct nova_app_catalog_reply {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int request_id;
    unsigned int index;
    unsigned int total_count;
    unsigned int result;

    unsigned int client_id;
    unsigned int state;
    unsigned int catalog_flags;
    unsigned int reserved;

    unsigned long long catalog_generation;

    char app_id[NOVA_APP_ID_BYTES];
    char display_name[NOVA_APP_DISPLAY_NAME_BYTES];
};

_Static_assert(
    sizeof(struct nova_app_catalog_request) == 32,
    "Nova application catalog request ABI size mismatch"
);

_Static_assert(
    sizeof(struct nova_app_catalog_reply) == 152,
    "Nova application catalog reply ABI size mismatch"
);

#endif
