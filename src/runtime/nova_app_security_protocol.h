#ifndef NOVA_APP_SECURITY_PROTOCOL_H
#define NOVA_APP_SECURITY_PROTOCOL_H

/*
 * NovaOS Stage-5A Phase-16 application security-principal/capability plane.
 *
 * This protocol is deliberately separate from the application catalog and
 * lifecycle planes. Catalog metadata describes applications; lifecycle IPC
 * starts/stops them; this plane binds a live process to a trusted application
 * principal and exposes the bounded capability policy that trusted services
 * may enforce.
 *
 * Security boundary:
 *  - An application-provided client_id is NEVER an authentication credential.
 *  - LOOKUP_PID is intended for a trusted service using a kernel-observed
 *    AF_UNIX peer PID (for example SO_PEERCRED), never a PID claimed by the
 *    untrusted application payload itself.
 *  - app_id + client_id + app_generation are attributes of the authenticated
 *    principal returned by novad.
 *  - effective_capabilities is the authorization set. It MUST be a subset of
 *    declared_capabilities and unknown bits fail closed.
 *  - executable paths, argv, environment strings, shell commands, arbitrary
 *    filesystem paths and URIs are never transported by this protocol.
 */

#include "nova_app_protocol.h"

#define NOVA_APP_SECURITY_SOCKET_PATH "/run/nova/app-security.sock"
#define NOVA_APP_SECURITY_PROTOCOL_MAGIC 0x4e534543U /* NSEC */
#define NOVA_APP_SECURITY_PROTOCOL_VERSION 1U

/*
 * Phase-16 capability vocabulary. Only resources mediated by NovaOS trusted
 * IPC are included here. Filesystem/network/device sandboxing is explicitly
 * deferred until there is a kernel-enforced process-isolation mechanism.
 */
enum {
    NOVA_APP_CAP_UI_SURFACE        = 1ULL << 0,
    NOVA_APP_CAP_INPUT_TEXT        = 1ULL << 1,
    NOVA_APP_CAP_INPUT_TAB_CAPTURE = 1ULL << 2,
    NOVA_APP_CAP_CLIPBOARD_READ    = 1ULL << 3,
    NOVA_APP_CAP_CLIPBOARD_WRITE   = 1ULL << 4,
};

#define NOVA_APP_CAP_PHASE16_KNOWN_MASK \
    (NOVA_APP_CAP_UI_SURFACE | NOVA_APP_CAP_INPUT_TEXT | \
     NOVA_APP_CAP_INPUT_TAB_CAPTURE | NOVA_APP_CAP_CLIPBOARD_READ | \
     NOVA_APP_CAP_CLIPBOARD_WRITE)

#define NOVA_APP_CAP_TOKEN_UI_SURFACE         "ui.surface"
#define NOVA_APP_CAP_TOKEN_INPUT_TEXT         "input.text"
#define NOVA_APP_CAP_TOKEN_INPUT_TAB_CAPTURE  "input.tab-capture"
#define NOVA_APP_CAP_TOKEN_CLIPBOARD_READ     "clipboard.read"
#define NOVA_APP_CAP_TOKEN_CLIPBOARD_WRITE    "clipboard.write"

/* Phase-18 Step-2: exactly one fixed, packaged, read-only broker resource. */
#define NOVA_APP_CAP_RESOURCE_DEMO_READ (1ULL << 5)
#define NOVA_APP_CAP_TOKEN_RESOURCE_DEMO_READ "resource.demo.read"
#define NOVA_APP_CAP_RESOURCE_NOTES_READ (1ULL << 6)
#define NOVA_APP_CAP_TOKEN_RESOURCE_NOTES_READ "resource.notes.read"
#define NOVA_APP_CAP_PHASE18_KNOWN_MASK \
    (NOVA_APP_CAP_PHASE16_KNOWN_MASK | NOVA_APP_CAP_RESOURCE_DEMO_READ)
#define NOVA_APP_CAP_PHASE18_STEP5_KNOWN_MASK \
    (NOVA_APP_CAP_PHASE18_KNOWN_MASK | NOVA_APP_CAP_RESOURCE_NOTES_READ)

enum {
    NOVA_APP_SECURITY_OP_HELLO = 1U,
    NOVA_APP_SECURITY_OP_LOOKUP_PID = 2U,

    NOVA_APP_SECURITY_OP_HELLO_REPLY = 0x100U,
    NOVA_APP_SECURITY_OP_PRINCIPAL = 0x101U,
    NOVA_APP_SECURITY_OP_ERROR = 0x102U,
};

enum {
    NOVA_APP_SECURITY_RESULT_OK = 0U,
    NOVA_APP_SECURITY_RESULT_BAD_REQUEST = 1U,
    NOVA_APP_SECURITY_RESULT_DENIED = 2U,
    NOVA_APP_SECURITY_RESULT_NOT_FOUND = 3U,
    NOVA_APP_SECURITY_RESULT_INTERNAL = 4U,
};

enum {
    NOVA_APP_SECURITY_FLAG_AUTHENTICATED = 1U << 0,
    NOVA_APP_SECURITY_FLAG_POLICY_BOUND = 1U << 1,
};

/*
 * HELLO requires process_id=0 and reserved fields zero.
 * LOOKUP_PID requires a non-zero kernel-observed peer process_id.
 *
 * The request intentionally contains neither app_id nor client_id. Those are
 * trusted outputs of principal resolution, not caller-selected credentials.
 */
struct nova_app_security_request {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int request_id;
    unsigned int process_id;
    unsigned int reserved0;
    unsigned int reserved1;
};

/*
 * HELLO_REPLY: identity/capability fields are zero/empty.
 * PRINCIPAL:   process_id, app identity, lifecycle generation and policy sets
 *              are valid and security_flags includes AUTHENTICATED.
 * ERROR:       result explains failure; identity/capability fields are empty.
 */
struct nova_app_security_reply {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;

    unsigned int request_id;
    unsigned int result;
    unsigned int process_id;
    unsigned int client_id;

    unsigned int security_flags;
    unsigned int reserved;

    unsigned long long app_generation;
    unsigned long long policy_generation;
    unsigned long long declared_capabilities;
    unsigned long long effective_capabilities;

    char app_id[NOVA_APP_ID_BYTES];
};

_Static_assert(sizeof(struct nova_app_security_request) == 32,
               "Nova application-security request ABI size mismatch");
_Static_assert(sizeof(struct nova_app_security_reply) == 104,
               "Nova application-security reply ABI size mismatch");

#endif
