#ifndef NOVA_APP_METADATA_PROTOCOL_H
#define NOVA_APP_METADATA_PROTOCOL_H

/*
 * NovaOS rich application metadata extension.
 *
 * This is an additive read-only extension to the accepted Phase-14
 * application-catalog plane. It deliberately keeps the Phase-14 catalog
 * protocol/version and its request/entry ABI unchanged. Older catalog clients
 * can continue to enumerate the base catalog without understanding metadata.
 *
 * Security boundary:
 *  - app_id remains the application identity and lifecycle authority key.
 *  - category, description and icon_asset_id are presentation/discovery data.
 *  - icon_asset_id is an opaque bounded asset identity, never a filesystem path.
 *  - executable paths, argv, environment strings, shell commands and arbitrary
 *    resource paths are never transported by this extension.
 */

#include "nova_app_catalog_protocol.h"

#define NOVA_APP_DESCRIPTION_BYTES 128U
#define NOVA_APP_ICON_ASSET_ID_BYTES 48U

/* Additive request/reply opcodes on NOVA_APP_CATALOG_SOCKET_PATH. */
#define NOVA_APP_CATALOG_OP_GET_METADATA 3U
#define NOVA_APP_CATALOG_OP_METADATA 0x104U

/* Presentation-only launcher categories. They are not authorization classes. */
enum {
    NOVA_APP_CATEGORY_UNSPECIFIED = 0U,
    NOVA_APP_CATEGORY_SYSTEM = 1U,
    NOVA_APP_CATEGORY_PRODUCTIVITY = 2U,
    NOVA_APP_CATEGORY_DEVELOPMENT = 3U,
    NOVA_APP_CATEGORY_MEDIA = 4U,
    NOVA_APP_CATEGORY_COMMUNICATION = 5U,
    NOVA_APP_CATEGORY_GAME = 6U,
    NOVA_APP_CATEGORY_UTILITY = 7U,
};

/* Presence only; no capability/permission semantics live in this bitset. */
enum {
    NOVA_APP_METADATA_FLAG_HAS_DESCRIPTION = 1U << 0,
    NOVA_APP_METADATA_FLAG_HAS_ICON = 1U << 1,
};

/*
 * GET_METADATA reuses struct nova_app_catalog_request:
 *   opcode = NOVA_APP_CATALOG_OP_GET_METADATA
 *   index  = zero-based catalog index
 *   flags/reserved = 0
 *
 * A successful reply binds richer presentation metadata back to the exact
 * base-catalog snapshot through index + total_count + catalog_generation +
 * app_id + client_id. Consumers must reject drift or identity mismatch.
 */
struct nova_app_metadata_reply {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int request_id;
    unsigned int index;
    unsigned int total_count;
    unsigned int result;

    unsigned int client_id;
    unsigned int category;
    unsigned int metadata_flags;
    unsigned int reserved;

    unsigned long long catalog_generation;

    char app_id[NOVA_APP_ID_BYTES];
    char description[NOVA_APP_DESCRIPTION_BYTES];
    char icon_asset_id[NOVA_APP_ICON_ASSET_ID_BYTES];
};

_Static_assert(
    sizeof(struct nova_app_catalog_request) == 32,
    "Phase-14 catalog request ABI must remain unchanged"
);

_Static_assert(
    sizeof(struct nova_app_catalog_reply) == 152,
    "Phase-14 catalog entry ABI must remain unchanged"
);

_Static_assert(
    sizeof(struct nova_app_metadata_reply) == 264,
    "Nova application metadata reply ABI size mismatch"
);

#endif
