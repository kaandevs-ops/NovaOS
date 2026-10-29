#ifndef NOVA_DRM_H
#define NOVA_DRM_H

#include "nova_linux.h"

/* Minimal DRM/KMS UAPI subset for NovaOS Stage-5A. NovaOS targets only
 * LP64 x86_64/aarch64, matching the Linux UAPI structure layout below. */
struct nova_drm_version {
    int version_major;
    int version_minor;
    int version_patchlevel;
    nova_size_t name_len;
    char *name;
    nova_size_t date_len;
    char *date;
    nova_size_t desc_len;
    char *desc;
};

struct nova_drm_mode_modeinfo {
    unsigned int clock;
    unsigned short hdisplay;
    unsigned short hsync_start;
    unsigned short hsync_end;
    unsigned short htotal;
    unsigned short hskew;
    unsigned short vdisplay;
    unsigned short vsync_start;
    unsigned short vsync_end;
    unsigned short vtotal;
    unsigned short vscan;
    unsigned int vrefresh;
    unsigned int flags;
    unsigned int type;
    char name[32];
};

struct nova_drm_mode_card_res {
    unsigned long long fb_id_ptr;
    unsigned long long crtc_id_ptr;
    unsigned long long connector_id_ptr;
    unsigned long long encoder_id_ptr;
    unsigned int count_fbs;
    unsigned int count_crtcs;
    unsigned int count_connectors;
    unsigned int count_encoders;
    unsigned int min_width;
    unsigned int max_width;
    unsigned int min_height;
    unsigned int max_height;
};

struct nova_drm_mode_crtc {
    unsigned long long set_connectors_ptr;
    unsigned int count_connectors;
    unsigned int crtc_id;
    unsigned int fb_id;
    unsigned int x;
    unsigned int y;
    unsigned int gamma_size;
    unsigned int mode_valid;
    struct nova_drm_mode_modeinfo mode;
};

struct nova_drm_mode_get_encoder {
    unsigned int encoder_id;
    unsigned int encoder_type;
    unsigned int crtc_id;
    unsigned int possible_crtcs;
    unsigned int possible_clones;
};

struct nova_drm_mode_get_connector {
    unsigned long long encoders_ptr;
    unsigned long long modes_ptr;
    unsigned long long props_ptr;
    unsigned long long prop_values_ptr;
    unsigned int count_modes;
    unsigned int count_props;
    unsigned int count_encoders;
    unsigned int encoder_id;
    unsigned int connector_id;
    unsigned int connector_type;
    unsigned int connector_type_id;
    unsigned int connection;
    unsigned int mm_width;
    unsigned int mm_height;
    unsigned int subpixel;
    unsigned int pad;
};

struct nova_drm_mode_fb_dirty_cmd {
    unsigned int fb_id;
    unsigned int flags;
    unsigned int color;
    unsigned int num_clips;
    unsigned long long clips_ptr;
};

struct nova_drm_mode_fb_cmd {
    unsigned int fb_id;
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int bpp;
    unsigned int depth;
    unsigned int handle;
};

struct nova_drm_mode_fb_cmd2 {
    unsigned int fb_id;
    unsigned int width;
    unsigned int height;
    unsigned int pixel_format;
    unsigned int flags;
    unsigned int handles[4];
    unsigned int pitches[4];
    unsigned int offsets[4];
    unsigned long long modifier[4];
};

struct nova_drm_mode_create_dumb {
    unsigned int height;
    unsigned int width;
    unsigned int bpp;
    unsigned int flags;
    unsigned int handle;
    unsigned int pitch;
    unsigned long long size;
};

struct nova_drm_mode_map_dumb {
    unsigned int handle;
    unsigned int pad;
    unsigned long long offset;
};

struct nova_drm_mode_destroy_dumb {
    unsigned int handle;
};

_Static_assert(sizeof(struct nova_drm_version) == 64, "DRM version ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_mode_modeinfo) == 68, "DRM modeinfo ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_mode_card_res) == 64, "DRM KMS resources ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_mode_crtc) == 104, "DRM CRTC ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_mode_get_encoder) == 20, "DRM encoder ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_mode_get_connector) == 80, "DRM connector ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_mode_fb_dirty_cmd) == 24, "DRM dirtyfb ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_mode_fb_cmd) == 28, "DRM framebuffer ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_mode_fb_cmd2) == 104, "DRM framebuffer2 ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_mode_create_dumb) == 32, "DRM dumb-create ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_mode_map_dumb) == 16, "DRM dumb-map ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_mode_destroy_dumb) == 4, "DRM dumb-destroy ABI size mismatch");

struct nova_drm_mode_crtc_page_flip {
    unsigned int crtc_id;
    unsigned int fb_id;
    unsigned int flags;
    unsigned int reserved;
    unsigned long long user_data;
};

struct nova_drm_event {
    unsigned int type;
    unsigned int length;
};

struct nova_drm_event_vblank {
    struct nova_drm_event base;
    unsigned long long user_data;
    unsigned int tv_sec;
    unsigned int tv_usec;
    unsigned int sequence;
    unsigned int crtc_id;
};

_Static_assert(sizeof(struct nova_drm_mode_crtc_page_flip) == 24, "DRM page-flip ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_event) == 8, "DRM event ABI size mismatch");
_Static_assert(sizeof(struct nova_drm_event_vblank) == 32, "DRM vblank event ABI size mismatch");

#define NOVA_IOC_NRBITS     8U
#define NOVA_IOC_TYPEBITS   8U
#define NOVA_IOC_SIZEBITS   14U
#define NOVA_IOC_NRSHIFT    0U
#define NOVA_IOC_TYPESHIFT  (NOVA_IOC_NRSHIFT + NOVA_IOC_NRBITS)
#define NOVA_IOC_SIZESHIFT  (NOVA_IOC_TYPESHIFT + NOVA_IOC_TYPEBITS)
#define NOVA_IOC_DIRSHIFT   (NOVA_IOC_SIZESHIFT + NOVA_IOC_SIZEBITS)
#define NOVA_IOC_WRITE      1U
#define NOVA_IOC_READ       2U
#define NOVA_IOC(dir,type,nr,size) \
    (((unsigned long)(dir) << NOVA_IOC_DIRSHIFT) | \
     ((unsigned long)(type) << NOVA_IOC_TYPESHIFT) | \
     ((unsigned long)(nr) << NOVA_IOC_NRSHIFT) | \
     ((unsigned long)(size) << NOVA_IOC_SIZESHIFT))
#define NOVA_DRM_IO(type,nr) NOVA_IOC(0U, (type), (nr), 0U)
#define NOVA_DRM_IOWR(nr,type) NOVA_IOC(NOVA_IOC_READ | NOVA_IOC_WRITE, 'd', (nr), sizeof(type))

#define NOVA_DRM_IOCTL_VERSION             NOVA_DRM_IOWR(0x00, struct nova_drm_version)
#define NOVA_DRM_IOCTL_SET_MASTER          NOVA_DRM_IO('d', 0x1e)
#define NOVA_DRM_IOCTL_MODE_GETRESOURCES   NOVA_DRM_IOWR(0xA0, struct nova_drm_mode_card_res)
#define NOVA_DRM_IOCTL_MODE_GETCRTC        NOVA_DRM_IOWR(0xA1, struct nova_drm_mode_crtc)
#define NOVA_DRM_IOCTL_MODE_SETCRTC        NOVA_DRM_IOWR(0xA2, struct nova_drm_mode_crtc)
#define NOVA_DRM_IOCTL_MODE_GETENCODER     NOVA_DRM_IOWR(0xA6, struct nova_drm_mode_get_encoder)
#define NOVA_DRM_IOCTL_MODE_GETCONNECTOR   NOVA_DRM_IOWR(0xA7, struct nova_drm_mode_get_connector)
#define NOVA_DRM_IOCTL_MODE_ADDFB          NOVA_DRM_IOWR(0xAE, struct nova_drm_mode_fb_cmd)
#define NOVA_DRM_IOCTL_MODE_ADDFB2         NOVA_DRM_IOWR(0xB8, struct nova_drm_mode_fb_cmd2)
#define NOVA_DRM_IOCTL_MODE_RMFB           NOVA_DRM_IOWR(0xAF, unsigned int)
#define NOVA_DRM_IOCTL_MODE_PAGE_FLIP       NOVA_DRM_IOWR(0xB0, struct nova_drm_mode_crtc_page_flip)
#define NOVA_DRM_IOCTL_MODE_DIRTYFB         NOVA_DRM_IOWR(0xB1, struct nova_drm_mode_fb_dirty_cmd)
#define NOVA_DRM_IOCTL_MODE_CREATE_DUMB    NOVA_DRM_IOWR(0xB2, struct nova_drm_mode_create_dumb)
#define NOVA_DRM_IOCTL_MODE_MAP_DUMB       NOVA_DRM_IOWR(0xB3, struct nova_drm_mode_map_dumb)
#define NOVA_DRM_IOCTL_MODE_DESTROY_DUMB   NOVA_DRM_IOWR(0xB4, struct nova_drm_mode_destroy_dumb)

_Static_assert(NOVA_DRM_IOCTL_VERSION == 0xc0406400UL, "DRM VERSION ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_SET_MASTER == 0x641eUL, "DRM SET_MASTER ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_GETRESOURCES == 0xc04064a0UL, "DRM GETRESOURCES ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_GETCRTC == 0xc06864a1UL, "DRM GETCRTC ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_SETCRTC == 0xc06864a2UL, "DRM SETCRTC ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_GETENCODER == 0xc01464a6UL, "DRM GETENCODER ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_GETCONNECTOR == 0xc05064a7UL, "DRM GETCONNECTOR ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_ADDFB == 0xc01c64aeUL, "DRM ADDFB ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_ADDFB2 == 0xc06864b8UL, "DRM ADDFB2 ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_RMFB == 0xc00464afUL, "DRM RMFB ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_PAGE_FLIP == 0xc01864b0UL, "DRM PAGE_FLIP ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_DIRTYFB == 0xc01864b1UL, "DRM DIRTYFB ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_CREATE_DUMB == 0xc02064b2UL, "DRM CREATE_DUMB ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_MAP_DUMB == 0xc01064b3UL, "DRM MAP_DUMB ioctl encoding mismatch");
_Static_assert(NOVA_DRM_IOCTL_MODE_DESTROY_DUMB == 0xc00464b4UL, "DRM DESTROY_DUMB ioctl encoding mismatch");

#define NOVA_DRM_MODE_CONNECTED       1U
#define NOVA_DRM_MODE_PAGE_FLIP_EVENT  (1U << 0)
#define NOVA_DRM_EVENT_FLIP_COMPLETE   0x02U
#define NOVA_DRM_MODE_TYPE_PREFERRED  (1U << 3)

#define NOVA_DRM_FOURCC(a,b,c,d) \
    ((unsigned int)(a) | ((unsigned int)(b) << 8) | ((unsigned int)(c) << 16) | ((unsigned int)(d) << 24))
#define NOVA_DRM_FORMAT_XRGB8888 NOVA_DRM_FOURCC('X', 'R', '2', '4')
_Static_assert(NOVA_DRM_FORMAT_XRGB8888 == 0x34325258U, "DRM XRGB8888 fourcc mismatch");

/* input-event ABI: _IOR('E', 0x01, int) */
#define NOVA_EVIOCGVERSION NOVA_IOC(NOVA_IOC_READ, 'E', 0x01, sizeof(int))
_Static_assert(NOVA_EVIOCGVERSION == 0x80044501UL, "EVIOCGVERSION ioctl encoding mismatch");

#endif
