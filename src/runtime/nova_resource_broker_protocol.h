#ifndef NOVA_RESOURCE_BROKER_PROTOCOL_H
#define NOVA_RESOURCE_BROKER_PROTOCOL_H

/*
 * NovaOS Stage-5A Phase-18 trusted resource-broker foundation.
 *
 * Application-facing transport:
 *   sandbox path: /run/nova/resource.sock
 *   host path:    /run/nova/app/resource.sock
 *
 * Authentication is never supplied by the application request.  The broker
 * observes the AF_UNIX peer PID with SO_PEERCRED and resolves that PID through
 * a novad-owned, broker-only live-principal authority channel.  app_id,
 * client_id, generation and policy are outputs of that trusted lookup.
 *
 * Step-1 HELLO remains a non-grant session. Step-2 adds one fixed packaged
 * read-only resource via broker-side manifest capability enforcement. There
 * are still no caller-selected paths, URIs, PIDs, client IDs or app IDs.
 */

#include "nova_app_protocol.h"

#define NOVA_RESOURCE_BROKER_APP_SOCKET_PATH "/run/nova/resource.sock"
#define NOVA_RESOURCE_BROKER_HOST_SOCKET_PATH "/run/nova/app/resource.sock"
#define NOVA_RESOURCE_BROKER_AUTHORITY_SOCKET_PATH "/run/nova/resource-authority.sock"
#define NOVA_RESOURCE_BROKER_STATE_PATH "/run/nova/resource-broker.state"
#define NOVA_RESOURCE_BROKER_DEMO_SOURCE "/System/Resources/phase18-broker-demo.txt"
#define NOVA_RESOURCE_BROKER_NOTES_SOURCE "/System/Resources/phase18-broker-notes.txt"
#define NOVA_RESOURCE_ID_DEMO 1U
#define NOVA_RESOURCE_ID_NOTES 2U

#define NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC 0x4e524252U /* NRBR */
#define NOVA_RESOURCE_BROKER_PROTOCOL_VERSION 1U

#define NOVA_RESOURCE_BROKER_MAX_CLIENTS 4U

enum {
    NOVA_RESOURCE_BROKER_OP_HELLO = 1U,
    /* Legacy opcode: no caller-supplied resource selector. */
    NOVA_RESOURCE_BROKER_OP_OPEN_DEMO_RO = 2U,
    NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO = 3U,
    NOVA_RESOURCE_BROKER_OP_GRANT_FD = 0x102U,

    NOVA_RESOURCE_BROKER_OP_SESSION = 0x100U,
    NOVA_RESOURCE_BROKER_OP_ERROR = 0x101U,
};

enum {
    NOVA_RESOURCE_BROKER_RESULT_OK = 0U,
    NOVA_RESOURCE_BROKER_RESULT_BAD_REQUEST = 1U,
    NOVA_RESOURCE_BROKER_RESULT_DENIED = 2U,
    NOVA_RESOURCE_BROKER_RESULT_UNSUPPORTED = 3U,
    NOVA_RESOURCE_BROKER_RESULT_INTERNAL = 4U,
};

enum {
    NOVA_RESOURCE_BROKER_FLAG_AUTHENTICATED = 1U << 0,
    NOVA_RESOURCE_BROKER_FLAG_POLICY_BOUND = 1U << 1,
    NOVA_RESOURCE_BROKER_FLAG_NO_GRANT_SURFACE = 1U << 2,
    NOVA_RESOURCE_BROKER_FLAG_FD_READONLY = 1U << 3,
};

/*
 * The request carries framing and a catalog selector for opcode 3.
 * Identity and grant authority MUST NOT be caller supplied.
 */
struct nova_resource_broker_request {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int request_id;
    unsigned int reserved0; /* catalog slot for opcode 3; zero otherwise */
    unsigned int reserved1;
    unsigned int reserved2;
};

/*
 * SESSION is an authenticated description of the kernel-observed peer.  It is
 * not a resource grant and carries no path, file descriptor, URI or device ID.
 */
struct nova_resource_broker_reply {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int request_id;
    unsigned int result;
    unsigned int broker_flags;
    unsigned int reserved;

    unsigned int process_id;
    unsigned int client_id;
    unsigned long long app_generation;
    unsigned long long policy_generation;
    unsigned long long effective_capabilities;

    char app_id[NOVA_APP_ID_BYTES];
};

_Static_assert(sizeof(struct nova_resource_broker_request) == 32,
               "Nova resource-broker request ABI size mismatch");
_Static_assert(sizeof(struct nova_resource_broker_reply) == 96,
               "Nova resource-broker reply ABI size mismatch");

#endif
