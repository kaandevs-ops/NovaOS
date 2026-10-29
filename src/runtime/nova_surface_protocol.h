#ifndef NOVA_SURFACE_PROTOCOL_H
#define NOVA_SURFACE_PROTOCOL_H

#define NOVA_SURFACE_SOCKET_PATH "/run/nova/client.sock"
#define NOVA_SURFACE_SANDBOX_SOCKET_DIR "/run/nova/app"
#define NOVA_SURFACE_SANDBOX_SOCKET_PATH "/run/nova/app/client.sock"
#define NOVA_SURFACE_PROTOCOL_MAGIC 0x4e535246U
#define NOVA_SURFACE_PROTOCOL_VERSION 6U
#define NOVA_SURFACE_MAX_CLIENT_SURFACES 8U
#define NOVA_SURFACE_MAX_CLIENT_BUFFERS 8U
#define NOVA_SURFACE_MAX_BUFFER_BYTES (8U * 1024U * 1024U)
#define NOVA_SURFACE_TITLE_BYTES 32U
#define NOVA_SURFACE_CLIPBOARD_BYTES 64U
#define NOVA_PIXEL_FORMAT_XRGB8888 0x34325258U

#define NOVA_TOPLEVEL_TITLEBAR_HEIGHT 28U
#define NOVA_TOPLEVEL_RESIZE_EDGE 10U
#define NOVA_TOPLEVEL_CLOSE_SIZE 18U

enum {
    NOVA_SURFACE_OP_HELLO = 1U,
    NOVA_SURFACE_OP_CREATE_SURFACE = 2U,
    NOVA_SURFACE_OP_SET_POSITION = 3U,
    NOVA_SURFACE_OP_ATTACH_BUFFER = 4U,
    NOVA_SURFACE_OP_DAMAGE = 5U,
    NOVA_SURFACE_OP_COMMIT = 6U,
    NOVA_SURFACE_OP_HEARTBEAT = 7U,
    NOVA_SURFACE_OP_CREATE_BUFFER = 8U,
    NOVA_SURFACE_OP_DESTROY_BUFFER = 9U,
    NOVA_SURFACE_OP_SET_ROLE = 10U,
    NOVA_SURFACE_OP_SET_TITLE = 11U,
    NOVA_SURFACE_OP_SET_SIZE_LIMITS = 12U,
    NOVA_SURFACE_OP_ACK_CONFIGURE = 13U,
    NOVA_SURFACE_OP_DESTROY_SURFACE = 14U,
    NOVA_SURFACE_OP_SET_INPUT_FLAGS = 15U,
    NOVA_SURFACE_OP_SET_CLIPBOARD = 16U,
    NOVA_SURFACE_OP_GET_CLIPBOARD = 17U,
    NOVA_SURFACE_OP_DRAG_PATH = 18U, /* opt-in gesture-bound Files path text; no FDs */
};

enum {
    NOVA_SURFACE_ROLE_NONE = 0U,
    NOVA_SURFACE_ROLE_TOPLEVEL = 1U,
};

enum {
    NOVA_SURFACE_INPUT_CAPTURE_TAB = 0x1U,
    NOVA_SURFACE_INPUT_TEXT_ASCII = 0x2U,
    NOVA_SURFACE_INPUT_CLIPBOARD = 0x4U,
};

enum {
    NOVA_INPUT_MOD_SHIFT = 0x1U,
    NOVA_INPUT_MOD_CTRL = 0x2U,
};

enum {
    NOVA_SURFACE_EVENT_FOCUS = 0x100U,
    NOVA_SURFACE_EVENT_KEY = 0x101U,
    NOVA_SURFACE_EVENT_POINTER = 0x102U,
    NOVA_SURFACE_EVENT_CONFIGURE = 0x103U,
    NOVA_SURFACE_EVENT_CLOSE_REQUEST = 0x104U,
    NOVA_SURFACE_EVENT_CLIPBOARD = 0x105U,
    NOVA_SURFACE_EVENT_LAUNCH = 0x106U,
};

struct nova_surface_message {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;
    unsigned int client_id;
    unsigned int surface_id;
    unsigned int sequence;
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    unsigned int z_index;
    unsigned int buffer_id;
    unsigned int pixel_format;
    unsigned int stride;
    unsigned long long buffer_bytes;
    int damage_x;
    int damage_y;
    unsigned int damage_width;
    unsigned int damage_height;
    unsigned int role;
    unsigned int configure_serial;
    unsigned int min_width;
    unsigned int min_height;
    unsigned int max_width;
    unsigned int max_height;
    unsigned int input_flags;
    char title[NOVA_SURFACE_TITLE_BYTES];
    unsigned int clipboard_length;
    char clipboard[NOVA_SURFACE_CLIPBOARD_BYTES];
};

struct nova_surface_event {
    unsigned int magic;
    unsigned int version;
    unsigned int type;
    unsigned int size;
    unsigned int client_id;
    unsigned int surface_id;
    unsigned int code;
    int value;
    unsigned int sequence;
    int cursor_x;
    int cursor_y;
    unsigned int configure_serial;
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    unsigned int modifiers;
    unsigned int clipboard_length;
    char clipboard[NOVA_SURFACE_CLIPBOARD_BYTES];
};

_Static_assert(sizeof(struct nova_surface_message) == 216, "Nova surface IPC message ABI size mismatch");
_Static_assert(sizeof(struct nova_surface_event) == 136, "Nova surface IPC event ABI size mismatch");

#endif
