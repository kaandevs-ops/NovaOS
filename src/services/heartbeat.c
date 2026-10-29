#include "nova_runtime.h"

int nova_main(void) {
    struct nova_timespec delay;
    delay.tv_sec = 30;
    delay.tv_nsec = 0;

    for (;;) {
        nova_nanosleep(&delay, (struct nova_timespec *)0);
    }
    return 0;
}
