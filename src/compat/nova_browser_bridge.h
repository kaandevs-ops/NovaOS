#ifndef NOVA_BROWSER_BRIDGE_H
#define NOVA_BROWSER_BRIDGE_H
/* Caller supplies an authenticated-transport candidate; compositor still
 * authenticates PID/principal via novad. This API grants no identity authority.
 * Fixed-size first integration; resize requires a later validated XRandR path. */
int nova_browser_bridge_run_supervised(int socket_fd,const char *private_display,int lifecycle_fd);
int nova_browser_bridge_run(int socket_fd,const char *private_display);
#endif
