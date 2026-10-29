#ifndef NOVA_FILES_UI_H
#define NOVA_FILES_UI_H
#include "nova_files_core.h"
#include "nova_files_metadata.h"
/* R20: fixed-size navigation history, no persistence or filesystem authority.
 * Left/Right traverse successful history; Home/End/Page keys navigate rows.
 * R15 UI is independent of display IPC and files sandbox policy.
 * Renderer writes only to a caller-owned validated XRGB8888 buffer. */
struct nova_files_ui;
int nova_files_ui_path_at(const struct nova_files_ui*,int,int,char*,unsigned);
#define NOVA_FILES_UI_WIDTH 800U
#define NOVA_FILES_UI_MIN_WIDTH 640U
#define NOVA_FILES_UI_HEIGHT 500U
#define NOVA_FILES_UI_MIN_HEIGHT 400U
#define NOVA_FILES_UI_FIRST_ROW_Y 128
#define NOVA_FILES_UI_ROW_H 40
#define NOVA_FILES_UI_MAX_ROWS 8U
#define NOVA_FILES_UI_LIST_X 227
#define NOVA_FILES_HISTORY 16U
struct nova_files_ui {
    struct nova_files_model *files;
    nova_files_metadata_reader metadata_reader;
    struct nova_files_metadata metadata[NOVA_FILES_MAX_ENTRIES];
    char history[NOVA_FILES_HISTORY][NOVA_FILES_PATH_BYTES];
    unsigned int history_count,history_index;
    unsigned int scroll;
    unsigned int selected;
    unsigned int refresh_generation;
    unsigned int last_error;
    unsigned int click_count;
    unsigned int folders_only,sort_mode;
    unsigned int theme,place_count,hover_control,pressed_control;
    char places[6][NOVA_FILES_NAME_BYTES];
    unsigned int workspace_shared;
    unsigned int properties_open;
    int (*preview_reader)(int,const char*,char*,unsigned);
    unsigned preview_open;int preview_status;char preview_text[4097];
    unsigned int search_focused;
    char name_filter[64];
    unsigned int hovered;
 unsigned int viewport_width,viewport_height;
};
static inline unsigned int nova_files_ui_sidebar_width(unsigned int w){return w<760U?156U:196U;}
void nova_files_ui_init(struct nova_files_ui *ui,struct nova_files_model *files);
int nova_files_ui_poll_metadata(struct nova_files_ui*);
void nova_files_ui_set_metadata(struct nova_files_ui*,nova_files_metadata_reader);
int nova_files_ui_resize(struct nova_files_ui *ui,unsigned int width,unsigned int height);
int nova_files_ui_pointer(struct nova_files_ui *ui,int x,int y,int button,int pressed);
int nova_files_ui_find(struct nova_files_ui *ui);
int nova_files_ui_filter_key(struct nova_files_ui *ui,unsigned int ascii);
unsigned int nova_files_ui_match_count(const struct nova_files_ui *ui);
int nova_files_ui_scroll(struct nova_files_ui *ui,int detents);
int nova_files_ui_key(struct nova_files_ui *ui,unsigned int key);
int nova_files_ui_render(const struct nova_files_ui *ui,void *pixels,
                         unsigned int width,unsigned int height,unsigned int pitch);
int nova_files_ui_open_desktop(struct nova_files_ui*u,const char*name);
#endif
