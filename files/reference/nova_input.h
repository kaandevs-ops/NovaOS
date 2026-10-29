#ifndef NOVA_INPUT_H
#define NOVA_INPUT_H

/* Minimal Linux input-event UAPI subset used by NovaOS Stage-5A. */
struct nova_input_event {
    long sec;
    long usec;
    unsigned short type;
    unsigned short code;
    int value;
};

_Static_assert(sizeof(struct nova_input_event) == 24, "input_event ABI size mismatch");

enum {
    NOVA_EV_SYN = 0x00,
    NOVA_EV_KEY = 0x01,
    NOVA_EV_REL = 0x02,
    NOVA_REL_X = 0x00,
    NOVA_REL_Y = 0x01,
    NOVA_KEY_BACKSPACE = 14,
    NOVA_KEY_TAB = 15,
    NOVA_KEY_Q = 16,
    NOVA_KEY_W = 17,
    NOVA_KEY_E = 18,
    NOVA_KEY_R = 19,
    NOVA_KEY_T = 20,
    NOVA_KEY_Y = 21,
    NOVA_KEY_U = 22,
    NOVA_KEY_I = 23,
    NOVA_KEY_O = 24,
    NOVA_KEY_P = 25,
    NOVA_KEY_ENTER = 28,
    NOVA_KEY_LEFTCTRL = 29,
    NOVA_KEY_A = 30,
    NOVA_KEY_S = 31,
    NOVA_KEY_D = 32,
    NOVA_KEY_F = 33,
    NOVA_KEY_G = 34,
    NOVA_KEY_H = 35,
    NOVA_KEY_J = 36,
    NOVA_KEY_K = 37,
    NOVA_KEY_L = 38,
    NOVA_KEY_LEFTSHIFT = 42,
    NOVA_KEY_Z = 44,
    NOVA_KEY_X = 45,
    NOVA_KEY_C = 46,
    NOVA_KEY_V = 47,
    NOVA_KEY_B = 48,
    NOVA_KEY_N = 49,
    NOVA_KEY_M = 50,
    NOVA_KEY_RIGHTSHIFT = 54,
    NOVA_KEY_SPACE = 57,
    NOVA_KEY_RIGHTCTRL = 97,
    NOVA_KEY_HOME = 102,
    NOVA_KEY_UP = 103,
    NOVA_KEY_LEFT = 105,
    NOVA_KEY_RIGHT = 106,
    NOVA_KEY_END = 107,
    NOVA_KEY_DOWN = 108,
    NOVA_KEY_DELETE = 111,
    NOVA_BTN_LEFT = 0x110,
};

#endif
