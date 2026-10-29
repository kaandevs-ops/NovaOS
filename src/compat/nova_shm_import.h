#ifndef NOVA_SHM_IMPORT_H
#define NOVA_SHM_IMPORT_H
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
/* Wayland wl_shm format values. This helper is NOT a Wayland server. */
#define NOVA_SHM_ARGB8888 0U
#define NOVA_SHM_XRGB8888 1U
struct nova_shm_spec {uint32_t width,height,stride,format;uint64_t offset,pool_bytes;};
/* Copies into unpublished caller scratch memory. Publish ONLY on success.
 * Never maps client memory (no SIGBUS on truncate), owns no fd or allocation.
 * pool fd must be seekable regular shm supporting F_GET_SEALS, owned by peer.
 * Concurrent writes can produce a torn image; no coherent-frame claim. */
int nova_shm_import(int fd,uid_t peer,const struct nova_shm_spec*s,uint32_t background,void*out,size_t capacity);
#endif
