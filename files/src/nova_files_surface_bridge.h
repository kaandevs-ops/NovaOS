#ifndef NOVA_FILES_SURFACE_BRIDGE_H
#define NOVA_FILES_SURFACE_BRIDGE_H
#include "nova_files_ui.h"
#include "../../src/runtime/nova_surface_protocol.h"
/* Compile-time ABI bridge ONLY. No socket, authentication or launcher here.
 * Caller must first validate owner/peer via novad and establish a surface. */
struct nova_files_surface_bridge {
 struct nova_files_ui *ui;
 unsigned int client_id;
 unsigned int surface_id;
 int surface_x, surface_y;
 unsigned int closed;
 unsigned int configured;
 unsigned int rejected;
 unsigned int theme_generation;
 unsigned int desktop_open_sequence;
};
void nova_files_surface_bridge_init(struct nova_files_surface_bridge *bridge,
 struct nova_files_ui *ui,unsigned int client_id,unsigned int surface_id,int x,int y);
/* returns 1 repaint, 0 no-op, -1 filesystem error, -2 invalid frame */
int nova_files_surface_bridge_event(struct nova_files_surface_bridge *bridge,
 const struct nova_surface_event *event);
#endif
