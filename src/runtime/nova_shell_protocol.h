#ifndef NOVA_SHELL_PROTOCOL_H
#define NOVA_SHELL_PROTOCOL_H

#define NOVA_SHELL_SOCKET_PATH "/run/nova/compositor.sock"
#define NOVA_SHELL_PROTOCOL_MAGIC 0x4e4f5641U
#define NOVA_SHELL_PROTOCOL_VERSION 1U

enum {
    NOVA_SHELL_OP_HELLO = 1U,
    NOVA_SHELL_OP_SCENE = 2U,
    NOVA_SHELL_OP_HEARTBEAT = 3U,
};

struct nova_shell_message {
    unsigned int magic;
    unsigned int version;
    unsigned int opcode;
    unsigned int size;
    unsigned int panel_height;
    int window_x;
    int window_y;
    unsigned int window_width;
    unsigned int window_height;
    unsigned int accent_rgb;
    unsigned int initial_focus;
    unsigned int sequence;
};

_Static_assert(sizeof(struct nova_shell_message) == 48, "Nova shell IPC ABI size mismatch");

#endif
