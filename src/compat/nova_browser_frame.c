#define _GNU_SOURCE
#include "nova_browser_frame.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
void nova_browser_frame_close(struct nova_browser_frame*f){if(!f)return;if(f->pixels&&f->pixels!=MAP_FAILED)munmap(f->pixels,f->bytes);free(f->scratch);if(f->fd>=0)close(f->fd);memset(f,0,sizeof*f);f->fd=-1;}
static int frame_create(struct nova_x11*x,struct nova_browser_frame*f,unsigned header){if(!x||!f)return -1;memset(f,0,sizeof*f);f->fd=-1;if(nova_x11_size(x,&f->width,&f->height))return -1;f->header_height=header;f->height+=header;uint64_t bytes=(uint64_t)f->width*f->height*4;if(!bytes||bytes>NOVA_SURFACE_MAX_BUFFER_BYTES)return -1;f->bytes=(unsigned long)bytes;f->fd=memfd_create("nova-browser-frame",MFD_CLOEXEC|MFD_ALLOW_SEALING);if(f->fd<0)goto fail;if(ftruncate(f->fd,(off_t)bytes))goto fail;f->pixels=mmap(NULL,(size_t)bytes,PROT_READ|PROT_WRITE,MAP_SHARED,f->fd,0);if(f->pixels==MAP_FAILED)goto fail;f->scratch=malloc((size_t)bytes);if(!f->scratch)goto fail;if(fcntl(f->fd,F_ADD_SEALS,F_SEAL_SHRINK|F_SEAL_GROW|F_SEAL_SEAL))goto fail;return 0;
 fail:nova_browser_frame_close(f);return -1;}
int nova_browser_frame_refresh(struct nova_x11*x,struct nova_browser_frame*f){if(!x||!f||f->fd<0||!f->pixels||!f->scratch||!f->bytes)return -1;if(nova_x11_pixels(x,f->scratch,f->bytes))return -1;size_t offset=(size_t)f->header_height*f->width*4;if(offset>=f->bytes)return -1;size_t content=f->bytes-offset;void*destination=(unsigned char*)f->pixels+offset;if(!memcmp(destination,f->scratch,content))return 0;memcpy(destination,f->scratch,content);return 1;}
int nova_browser_frame_capture(struct nova_x11*x,struct nova_browser_frame*f){return nova_browser_frame_refresh(x,f)<0?-1:0;}
int nova_browser_frame_message(const struct nova_browser_frame*f,unsigned client,unsigned buffer,unsigned sequence,struct nova_surface_message*out){if(!f||f->fd<0||!out||!client||!buffer||!sequence||!f->width||!f->height||f->bytes!=(unsigned long)f->width*f->height*4||f->bytes>NOVA_SURFACE_MAX_BUFFER_BYTES)return -1;memset(out,0,sizeof*out);out->magic=NOVA_SURFACE_PROTOCOL_MAGIC;out->version=NOVA_SURFACE_PROTOCOL_VERSION;out->size=sizeof*out;out->opcode=NOVA_SURFACE_OP_CREATE_BUFFER;out->client_id=client;out->buffer_id=buffer;out->sequence=sequence;out->width=f->width;out->height=f->height;out->stride=f->width*4;out->buffer_bytes=f->bytes;out->pixel_format=NOVA_PIXEL_FORMAT_XRGB8888;return 0;}

int nova_browser_frame_create(struct nova_x11*x,struct nova_browser_frame*f){return frame_create(x,f,0);}
int nova_browser_frame_create_decorated(struct nova_x11*x,struct nova_browser_frame*f){return frame_create(x,f,NOVA_TOPLEVEL_TITLEBAR_HEIGHT);}
