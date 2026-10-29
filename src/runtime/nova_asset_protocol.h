#ifndef NOVA_ASSET_PROTOCOL_H
#define NOVA_ASSET_PROTOCOL_H

/*
 * NovaOS trusted asset service IPC.
 *
 * The asset service resolves opaque, bounded asset identities to immutable
 * presentation resources. Filesystem paths and URI-like resource references
 * are deliberately absent from this ABI.
 *
 * Security boundary:
 *  - asset_id is an opaque resource identity, not a path.
 *  - the service is read-only; there are no create/update/delete operations.
 *  - payload transfer is chunked and bounded.
 *  - application lifecycle authority remains outside this protocol.
 */

#define NOVA_ASSET_SOCKET_PATH "/run/nova/assets.sock"
#define NOVA_ASSET_STATE_PATH "/run/nova/assets.state"
#define NOVA_ASSET_PROTOCOL_MAGIC 0x4e415354U /* NAST */
#define NOVA_ASSET_PROTOCOL_VERSION 1U
#define NOVA_ASSET_ID_BYTES 48U
#define NOVA_ASSET_DIGEST_BYTES 32U
#define NOVA_ASSET_MAX_ENTRIES 64U
#define NOVA_ASSET_MAX_BYTES 16384U
#define NOVA_ASSET_CHUNK_BYTES 1024U

/* Read-only request opcodes. */
enum {
    NOVA_ASSET_OP_HELLO = 1U,
    NOVA_ASSET_OP_GET_INFO = 2U,
    NOVA_ASSET_OP_READ = 3U,

    NOVA_ASSET_OP_HELLO_REPLY = 0x100U,
    NOVA_ASSET_OP_INFO = 0x101U,
    NOVA_ASSET_OP_DATA = 0x102U,
    NOVA_ASSET_OP_ERROR = 0x103U,
};

enum {
    NOVA_ASSET_RESULT_OK = 0U,
    NOVA_ASSET_RESULT_BAD_REQUEST = 1U,
    NOVA_ASSET_RESULT_UNKNOWN_ASSET = 2U,
    NOVA_ASSET_RESULT_RANGE = 3U,
    NOVA_ASSET_RESULT_INTERNAL = 4U,
};

enum {
    NOVA_ASSET_FORMAT_A8_MASK = 1U,
};

enum {
    NOVA_ASSET_FLAG_IMMUTABLE = 1U << 0,
    NOVA_ASSET_FLAG_BUILTIN = 1U << 1,
};

struct nova_asset_request {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int request_id;
    unsigned int offset;
    unsigned int length;
    unsigned int flags;

    char asset_id[NOVA_ASSET_ID_BYTES];
};

/*
 * INFO carries immutable metadata and digest with chunk_bytes=0.
 * DATA carries at most NOVA_ASSET_CHUNK_BYTES bytes beginning at offset.
 * HELLO_REPLY carries protocol/registry information with an empty asset_id.
 * ERROR carries result and no trusted payload.
 */
struct nova_asset_reply {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int request_id;
    unsigned int result;
    unsigned int format;
    unsigned int asset_flags;

    unsigned int width;
    unsigned int height;
    unsigned int total_bytes;
    unsigned int offset;

    unsigned int chunk_bytes;
    unsigned int registry_count;

    unsigned char digest[NOVA_ASSET_DIGEST_BYTES];
    char asset_id[NOVA_ASSET_ID_BYTES];
    unsigned char payload[NOVA_ASSET_CHUNK_BYTES];
};

_Static_assert(sizeof(struct nova_asset_request) == 80,
               "Nova asset request ABI size mismatch");
_Static_assert(sizeof(struct nova_asset_reply) == 1160,
               "Nova asset reply ABI size mismatch");
_Static_assert(NOVA_ASSET_ID_BYTES == 48U,
               "Nova asset identity width must match Phase-15 icon identities");

#endif
