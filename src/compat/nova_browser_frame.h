#ifndef NOVA_BROWSER_FRAME_H
#define NOVA_BROWSER_FRAME_H
#include "nova_x11_pixels.h"
#include "nova_surface_protocol.h"
struct nova_browser_frame {int fd;void*pixels;void*scratch;unsigned width,height;unsigned long bytes;unsigned header_height;};
int nova_browser_frame_create_decorated(struct nova_x11*x,struct nova_browser_frame*f);
int nova_browser_frame_create(struct nova_x11*x,struct nova_browser_frame*f);
/* Refresh returns 1 for changed pixels, 0 unchanged, -1 failure. */
int nova_browser_frame_refresh(struct nova_x11*x,struct nova_browser_frame*f);
int nova_browser_frame_capture(struct nova_x11*x,struct nova_browser_frame*f);
void nova_browser_frame_close(struct nova_browser_frame*f);
/* Prepare native IPC metadata; authority/connection authentication remains caller responsibility. */
int nova_browser_frame_message(const struct nova_browser_frame*f,unsigned client,unsigned buffer,unsigned sequence,struct nova_surface_message*out);
#endif
