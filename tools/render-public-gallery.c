#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main

/* Production CPU renderer, bounded host-only UI states. No guest or app peer. */
static void capture(const char *dir, const char *name) {
    const unsigned w = 1600, h = 900;
    unsigned *pixels = calloc((size_t)w * h, 4);
    assert(pixels);
    draw_scene(pixels, w, h, w * 4, 0);
    char path[4096];
    assert(snprintf(path, sizeof path, "%s/%s.HOST-NOT-VM.ppm", dir, name) < (int)sizeof path);
    FILE *f = fopen(path, "wb");
    assert(f);
    fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (unsigned i = 0; i < w * h; i++) {
        unsigned char rgb[] = {pixels[i] >> 16, pixels[i] >> 8, pixels[i]};
        assert(fwrite(rgb, 1, 3, f) == 3);
    }
    assert(!fclose(f));
    free(pixels);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    g_scene.configured = 1;
    g_scene.panel_height = 38;
    g_render.theme = 1; /* Existing procedural Graphite wave, no photo. */
    g_r5_scale = 1;
    g_r5_logical_w = g_r2_canvas_width = 1600;
    g_r5_logical_h = g_r2_canvas_height = 900;
    g_render.cursor_x = 760;
    g_render.cursor_y = 500;
    assert(r21_set_theme(2));
    capture(argv[1], "01-desktop");
    g_r7_library_visible = 1;
    capture(argv[1], "02-applications");
    g_r7_library_visible = 0;
    g_r7_controls_visible = 1;
    g_settings_page = 0;
    capture(argv[1], "03-appearance");
    assert(r21_set_theme(0));
    capture(argv[1], "04-calm-light");
    assert(r21_set_theme(1));
    capture(argv[1], "05-graphite");
    assert(r21_set_theme(2));
    g_r7_controls_visible = 2;
    capture(argv[1], "06-quick-settings");
    g_r7_controls_visible = 0;
    g_sw.visible = g_sw.focused = 1;
    capture(argv[1], "07-software-no-peer");
    puts("PUBLIC_NATIVE_GALLERY_PASS: production renderer, host UI states, no guest/app peer");
    return 0;
}
