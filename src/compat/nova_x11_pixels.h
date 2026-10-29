#ifndef NOVA_X11_PIXELS_H
#define NOVA_X11_PIXELS_H
#include <stddef.h>
struct nova_x11;
struct nova_x11*nova_x11_open(const char*display);
int nova_x11_request_close(struct nova_x11*c);
void nova_x11_release_inputs(struct nova_x11*c);
void nova_x11_close(struct nova_x11*c);
int nova_x11_size(struct nova_x11*c,unsigned*w,unsigned*h);
/* Caller-owned unpublished XRGB scratch, bounded to native surface limit. */
int nova_x11_pixels(struct nova_x11*c,void*out,size_t capacity);
int nova_x11_key(struct nova_x11*c,unsigned evdev,int pressed);
int nova_x11_pointer(struct nova_x11*c,int x,int y,unsigned button,int pressed);
#endif
