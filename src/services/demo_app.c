#include "nova_theme.h"
#include "nova_runtime.h"
#include "nova_state_buffer.h"
#define nova_fd_puts nova_state_puts
#define nova_fd_putln nova_state_putln
#define nova_fd_put_u64 nova_state_put_u64
#include "nova_ui.h"
#include "nova_ui_font.h"
#include "../../files/src/nova_files_font.h"
#include "nova_input.h"
#include "nova_font.h"
#include "nova_textedit.h"
#include "nova_widget.h"
#include "nova_surface_protocol.h"

#ifdef NOVA_DEMO_APP_B
#define APP_STATE "/run/nova/demo-app-b.state"
#define APP_CLIENT_ID 2U
#define SURFACE_A 200U
#define SURFACE_B 201U
#define APP_PRIMARY_X 180
#define APP_PRIMARY_Y 220
#define APP_SECONDARY_X 320
#define APP_SECONDARY_Y 300
#define APP_INITIAL_TEXT "BETA"
#define APP_TITLE_A "Nova App B"
#define APP_TITLE_B "Nova App B Aux"
#define APP_START_LOG "[nova-demo-app-b] NovaOS Stage-5A Phase-11 independent client starting"
#define APP_CONNECTED_LOG "[nova-demo-app-b] stage=CONNECTED protocol=6 client=2 surfaces=2 buffers=2 isolation=owner-fd-client-id"
#define APP_GRACEFUL_EXIT_LOG "[nova-demo-app-b] process_lifecycle=GRACEFUL_EXIT reason=all-toplevels-closed"
#define APP_STOPPED_LOG "[nova-demo-app-b] process_lifecycle=STOPPED exit=0"
#define BUFFER_A_BG 0x003c2858U
#define BUFFER_A_ACCENT 0x0000ccccU
#define BUFFER_A_HEADER 0x00884fd1U
#define BUFFER_B_BG 0x00204048U
#define BUFFER_B_ACCENT 0x00ff8800U
#define BUFFER_B_HEADER 0x0000aaccU
#else
#define APP_STATE "/run/nova/demo-app.state"
#define APP_CLIENT_ID 1U
#define SURFACE_A 100U
#define SURFACE_B 101U
#define APP_PRIMARY_X 700
#define APP_PRIMARY_Y 380
#define APP_SECONDARY_X 820
#define APP_SECONDARY_Y 460
#define APP_INITIAL_TEXT "NOVA"
#define APP_TITLE_A "Nova Buffer A"
#define APP_TITLE_B "Nova Buffer B"
#define APP_START_LOG "[nova-demo-app] NovaOS Stage-5A Phase-11 independent client starting"
#define APP_CONNECTED_LOG "[nova-demo-app] stage=CONNECTED protocol=6 surfaces=2 buffers=2 roles=toplevel widgets=3 textedit=selection-clipboard transport=memfd-scm-rights client=1 isolation=owner-fd-client-id"
#define APP_GRACEFUL_EXIT_LOG "[nova-demo-app] process_lifecycle=GRACEFUL_EXIT reason=all-toplevels-closed"
#define APP_STOPPED_LOG "[nova-demo-app] process_lifecycle=STOPPED exit=0"
#define BUFFER_A_BG 0x00184878U
#define BUFFER_A_ACCENT 0x00ffcc00U
#define BUFFER_A_HEADER 0x0000aaccU
#define BUFFER_B_BG 0x00502860U
#define BUFFER_B_ACCENT 0x0000ccccU
#define BUFFER_B_HEADER 0x00cc55ddU
#endif
#define BUFFER_A 1U
#define BUFFER_B 2U
#define SURFACE_A_WIDTH 320U
#define SURFACE_A_HEIGHT 220U
#define SURFACE_B_WIDTH 300U
#define SURFACE_B_HEIGHT 200U
#define TILE_SIZE 64U
#define TILE_Y_A 40U
#define TILE_Y_B 56U
#define UI_TEXT_COLOR 0x00ffffffU
#define UI_MUTED_COLOR 0x00b8c4d4U
#define UI_BUTTON_BG 0x00303d4fU
#define UI_BUTTON_BORDER 0x006e86a7U
#define UI_BUTTON_X 24U
#define UI_BUTTON_WIDTH 156U
#define UI_BUTTON_HEIGHT 30U
#define UI_TEXT_X 24U
#define UI_HELLO_Y 116U
#define UI_COUNTER_Y 140U
#define UI_TEXT_FIELD_X 24U
#define UI_TEXT_FIELD_Y 156U
#define UI_TEXT_FIELD_WIDTH 156U
#define UI_TEXT_FIELD_HEIGHT 16U
#define UI_TEXT_FIELD_BG 0x001f2b3aU
#define UI_TEXT_FIELD_BORDER 0x007a8898U
#define UI_WIDGET_FOCUS 0x00ffffffU
#define UI_SELECTION_BG 0x005a6879U
#define APP_WIDGET_ROOT 1U
#define APP_WIDGET_TEXT_FIELD 2U
#define APP_WIDGET_BUTTON 3U

struct app_buffer {
    int fd;
    void *map;
    unsigned int id;
    unsigned int surface_id;
    unsigned int width;
    unsigned int height;
    unsigned int stride;
    unsigned long long bytes;
    unsigned int background;
    unsigned int accent;
    unsigned int header;
    unsigned int tile_x;
    unsigned int tile_y;
};

struct app_state {
    unsigned long reconnects;
    unsigned long messages_sent;
    unsigned long commits;
    unsigned long buffers_created;
    unsigned long buffers_destroyed;
    unsigned long buffer_updates;
    unsigned long focus_events;
    unsigned long key_events;
    unsigned long pointer_events;
    unsigned long configure_events;
    unsigned long configure_acks;
    unsigned long resize_buffer_replacements;
    unsigned long close_requests;
    unsigned long surfaces_closed;
    unsigned long launch_events;
    unsigned long window_reopens;
    unsigned long text_redraws;
    unsigned long button_activations;
    unsigned long keyboard_activations;
    unsigned long glyphs_drawn;
    unsigned long utf8_replacements;
    unsigned long layout_generation;
    unsigned long widget_focus_changes;
    unsigned long tab_navigations;
    unsigned long text_field_clicks;
    unsigned long text_edits;
    unsigned long text_inserts;
    unsigned long text_backspaces;
    unsigned long text_deletes;
    unsigned long caret_moves;
    unsigned long selection_changes;
    unsigned long clipboard_copies;
    unsigned long clipboard_cuts;
    unsigned long clipboard_paste_requests;
    unsigned long clipboard_pastes;
    unsigned long clipboard_events;
    unsigned int clipboard_last_length;
    unsigned int counter;
    unsigned int focus_surface;
    unsigned int next_buffer_id;
    int surface_a_x;
    int surface_a_y;
    int surface_b_x;
    int surface_b_y;
    unsigned int surface_a_width;
    unsigned int surface_a_height;
    unsigned int surface_b_width;
    unsigned int surface_b_height;
    unsigned int tile_a_x;
    unsigned int tile_b_x;
    unsigned int buffer_a_id;
    unsigned int buffer_b_id;
    int surface_a_closed;
    int surface_b_closed;
};

static struct app_state g_app;
static struct nova_widget_tree g_widgets;
static struct nova_textedit g_text;

static int pointer_is_error(const void *ptr) { return (unsigned long)ptr >= (unsigned long)-4095L; }

static void fill_addr(struct nova_sockaddr_un *addr) {
    const char path[] = NOVA_SURFACE_SOCKET_PATH;
    nova_zero(addr, sizeof(*addr)); addr->sun_family = NOVA_AF_UNIX;
    for (unsigned long i = 0; i < sizeof(path) && i < sizeof(addr->sun_path); i++) addr->sun_path[i] = path[i];
}

static unsigned int live_surface_count(void) {
    unsigned int n=0; if (!g_app.surface_a_closed) n++; if (!g_app.surface_b_closed) n++; return n;
}

static void write_state(const char *stage) {
    long fd = nova_open_write(APP_STATE, 0644); if (fd < 0) return;
    if (!nova_state_begin((int)fd)) {nova_close((int)fd);return;}
    nova_fd_putln((int)fd, "schema=4");
    nova_fd_puts((int)fd, "stage="); nova_fd_putln((int)fd, stage);
    nova_fd_puts((int)fd, "protocol="); nova_fd_put_u64((int)fd, NOVA_SURFACE_PROTOCOL_VERSION); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "client_id="); nova_fd_put_u64((int)fd, APP_CLIENT_ID); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "primary_surface_id="); nova_fd_put_u64((int)fd, SURFACE_A); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "secondary_surface_id="); nova_fd_put_u64((int)fd, SURFACE_B); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surfaces="); nova_fd_put_u64((int)fd, live_surface_count()); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "reconnects="); nova_fd_put_u64((int)fd, g_app.reconnects); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "messages_sent="); nova_fd_put_u64((int)fd, g_app.messages_sent); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "commits="); nova_fd_put_u64((int)fd, g_app.commits); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "buffers_created="); nova_fd_put_u64((int)fd, g_app.buffers_created); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "buffers_destroyed="); nova_fd_put_u64((int)fd, g_app.buffers_destroyed); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "buffer_updates="); nova_fd_put_u64((int)fd, g_app.buffer_updates); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "configure_events="); nova_fd_put_u64((int)fd, g_app.configure_events); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "configure_acks="); nova_fd_put_u64((int)fd, g_app.configure_acks); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "resize_buffer_replacements="); nova_fd_put_u64((int)fd, g_app.resize_buffer_replacements); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "close_requests="); nova_fd_put_u64((int)fd, g_app.close_requests); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surfaces_closed="); nova_fd_put_u64((int)fd, g_app.surfaces_closed); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "launch_events="); nova_fd_put_u64((int)fd, g_app.launch_events); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "window_reopens="); nova_fd_put_u64((int)fd, g_app.window_reopens); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "text_redraws="); nova_fd_put_u64((int)fd, g_app.text_redraws); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "button_activations="); nova_fd_put_u64((int)fd, g_app.button_activations); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "keyboard_activations="); nova_fd_put_u64((int)fd, g_app.keyboard_activations); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "glyphs_drawn="); nova_fd_put_u64((int)fd, g_app.glyphs_drawn); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "utf8_replacements="); nova_fd_put_u64((int)fd, g_app.utf8_replacements); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "layout_generation="); nova_fd_put_u64((int)fd, g_app.layout_generation); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "widget_count="); nova_fd_put_u64((int)fd, g_widgets.count); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "widget_focus="); nova_fd_put_u64((int)fd, g_widgets.focused_id); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "widget_focus_changes="); nova_fd_put_u64((int)fd, g_app.widget_focus_changes); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "tab_navigations="); nova_fd_put_u64((int)fd, g_app.tab_navigations); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "text_field_clicks="); nova_fd_put_u64((int)fd, g_app.text_field_clicks); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "text_edits="); nova_fd_put_u64((int)fd, g_app.text_edits); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "text_inserts="); nova_fd_put_u64((int)fd, g_app.text_inserts); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "text_backspaces="); nova_fd_put_u64((int)fd, g_app.text_backspaces); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "text_deletes="); nova_fd_put_u64((int)fd, g_app.text_deletes); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "caret_moves="); nova_fd_put_u64((int)fd, g_app.caret_moves); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "selection_changes="); nova_fd_put_u64((int)fd, g_app.selection_changes); nova_fd_putln((int)fd, "");
    { unsigned int ss=0U,se=0U; nova_textedit_selection_bounds(&g_text,&ss,&se);
      nova_fd_puts((int)fd, "selection_start="); nova_fd_put_u64((int)fd, ss); nova_fd_putln((int)fd, "");
      nova_fd_puts((int)fd, "selection_end="); nova_fd_put_u64((int)fd, se); nova_fd_putln((int)fd, "");
      nova_fd_puts((int)fd, "selection_length="); nova_fd_put_u64((int)fd, se-ss); nova_fd_putln((int)fd, ""); }
    nova_fd_puts((int)fd, "clipboard_copies="); nova_fd_put_u64((int)fd, g_app.clipboard_copies); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "clipboard_cuts="); nova_fd_put_u64((int)fd, g_app.clipboard_cuts); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "clipboard_paste_requests="); nova_fd_put_u64((int)fd, g_app.clipboard_paste_requests); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "clipboard_pastes="); nova_fd_put_u64((int)fd, g_app.clipboard_pastes); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "clipboard_events="); nova_fd_put_u64((int)fd, g_app.clipboard_events); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "clipboard_last_length="); nova_fd_put_u64((int)fd, g_app.clipboard_last_length); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "text_length="); nova_fd_put_u64((int)fd, g_text.length); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "caret_index="); nova_fd_put_u64((int)fd, g_text.caret); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "text_value="); nova_fd_putln((int)fd, g_text.bytes);
    nova_fd_putln((int)fd, "text_input_subset=ascii-utf8-selection-clipboard");
    nova_fd_puts((int)fd, "counter="); nova_fd_put_u64((int)fd, g_app.counter); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "button_y="); nova_fd_put_u64((int)fd, g_app.surface_a_height>UI_BUTTON_HEIGHT+16U?g_app.surface_a_height-UI_BUTTON_HEIGHT-16U:0U); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "text_field_y=");
    { unsigned int by=g_app.surface_a_height>UI_BUTTON_HEIGHT+16U?g_app.surface_a_height-UI_BUTTON_HEIGHT-16U:0U;
      nova_fd_put_u64((int)fd, by>UI_TEXT_FIELD_HEIGHT+2U?by-UI_TEXT_FIELD_HEIGHT-2U:0U); }
    nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "focus_events="); nova_fd_put_u64((int)fd, g_app.focus_events); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "key_events="); nova_fd_put_u64((int)fd, g_app.key_events); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "pointer_events="); nova_fd_put_u64((int)fd, g_app.pointer_events); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "focus_surface="); nova_fd_put_u64((int)fd, g_app.focus_surface); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface100_x="); nova_fd_put_u64((int)fd, g_app.surface_a_x<0?0UL:(unsigned long)g_app.surface_a_x); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface100_y="); nova_fd_put_u64((int)fd, g_app.surface_a_y<0?0UL:(unsigned long)g_app.surface_a_y); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface100_width="); nova_fd_put_u64((int)fd, g_app.surface_a_width); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface100_height="); nova_fd_put_u64((int)fd, g_app.surface_a_height); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface101_x="); nova_fd_put_u64((int)fd, g_app.surface_b_x<0?0UL:(unsigned long)g_app.surface_b_x); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface101_y="); nova_fd_put_u64((int)fd, g_app.surface_b_y<0?0UL:(unsigned long)g_app.surface_b_y); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface101_width="); nova_fd_put_u64((int)fd, g_app.surface_b_width); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface101_height="); nova_fd_put_u64((int)fd, g_app.surface_b_height); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface100_closed="); nova_fd_put_u64((int)fd, g_app.surface_a_closed?1UL:0UL); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface101_closed="); nova_fd_put_u64((int)fd, g_app.surface_b_closed?1UL:0UL); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "buffer1_tile_x="); nova_fd_put_u64((int)fd, g_app.tile_a_x); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "buffer2_tile_x="); nova_fd_put_u64((int)fd, g_app.tile_b_x); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface100_buffer_id="); nova_fd_put_u64((int)fd, g_app.buffer_a_id); nova_fd_putln((int)fd, "");
    nova_fd_puts((int)fd, "surface101_buffer_id="); nova_fd_put_u64((int)fd, g_app.buffer_b_id); nova_fd_putln((int)fd, "");
    (void)nova_state_end();
    (void)nova_fsync((int)fd); (void)nova_close((int)fd);
}

static void fill_buffer(struct app_buffer *buf, unsigned int color) {
    unsigned char *base=(unsigned char *)buf->map;
    for (unsigned int y=0;y<buf->height;y++) {
        unsigned int *row=(unsigned int *)(base+(unsigned long)y*buf->stride);
        for (unsigned int x=0;x<buf->width;x++) row[x]=color;
    }
}


static unsigned int button_y_for_height(unsigned int height) {
    return height>UI_BUTTON_HEIGHT+16U ? height-UI_BUTTON_HEIGHT-16U : 0U;
}

static unsigned int text_field_y_for_height(unsigned int height) {
    unsigned int by=button_y_for_height(height);
    return by>UI_TEXT_FIELD_HEIGHT+2U ? by-UI_TEXT_FIELD_HEIGHT-2U : 0U;
}

static unsigned int draw_text_clipped(struct app_buffer*buf,int x,int y,unsigned int scale,unsigned int color,const char*text,int cx,int cy,unsigned int cw,unsigned int ch){
 struct nova_ui_canvas canvas={(unsigned char*)buf->map,buf->width,buf->height,buf->stride};unsigned int role=scale>1?NOVA_UI_TITLE:NOVA_UI_BODY;
 nova_ui_text(canvas,x,y+nova_ui_face_for(role,1)->ascent,role,1,color,text,cx,cy,cw,ch);
 unsigned int count=0,pos=0;while(text[pos]&&pos<128){unsigned int cp,n=nova_ui_utf8((const unsigned char*)text+pos,128-pos,&cp);if(!n)break;if(nova_ui_faces[0].glyphs[nova_ui_glyph_index(cp)].cp!=cp)g_app.utf8_replacements++;pos+=n;count++;}
 g_app.glyphs_drawn+=count;return count;
}
static unsigned int draw_text(struct app_buffer*buf,int x,int y,unsigned int scale,unsigned int color,const char*text){return draw_text_clipped(buf,x,y,scale,color,text,0,0,buf->width,buf->height);}
static unsigned int text_prefix_width(unsigned int bytes){
 char prefix[NOVA_TEXTEDIT_CAPACITY];unsigned int n=bytes<g_text.length?bytes:g_text.length;if(n>=sizeof(prefix))n=sizeof(prefix)-1;
 for(unsigned int i=0;i<n;i++)prefix[i]=g_text.bytes[i];prefix[n]=0;return nova_ui_measure(prefix,NOVA_UI_CAPTION,1);
}
static void init_widget_tree(unsigned int width, unsigned int height) {
    nova_widget_tree_init(&g_widgets);
    unsigned int by=button_y_for_height(height), fy=text_field_y_for_height(height);
    (void)nova_widget_add(&g_widgets,APP_WIDGET_ROOT,NOVA_WIDGET_ROOT,0U,0,0,width,height,0U);
    (void)nova_widget_add(&g_widgets,APP_WIDGET_TEXT_FIELD,NOVA_WIDGET_TEXT_FIELD,APP_WIDGET_ROOT,
                          (int)UI_TEXT_FIELD_X,(int)fy,UI_TEXT_FIELD_WIDTH,UI_TEXT_FIELD_HEIGHT,NOVA_WIDGET_FLAG_FOCUSABLE);
    (void)nova_widget_add(&g_widgets,APP_WIDGET_BUTTON,NOVA_WIDGET_BUTTON,APP_WIDGET_ROOT,
                          (int)UI_BUTTON_X,(int)by,UI_BUTTON_WIDTH,UI_BUTTON_HEIGHT,NOVA_WIDGET_FLAG_FOCUSABLE);
    (void)nova_widget_focus_first(&g_widgets);
}

static void update_widget_layout(unsigned int width, unsigned int height) {
    struct nova_widget_node *root=nova_widget_find(&g_widgets,APP_WIDGET_ROOT);
    struct nova_widget_node *field=nova_widget_find(&g_widgets,APP_WIDGET_TEXT_FIELD);
    struct nova_widget_node *button=nova_widget_find(&g_widgets,APP_WIDGET_BUTTON);
    unsigned int by=button_y_for_height(height), fy=text_field_y_for_height(height);
    unsigned int field_avail=width>UI_TEXT_FIELD_X?width-UI_TEXT_FIELD_X:1U;
    unsigned int button_avail=width>UI_BUTTON_X?width-UI_BUTTON_X:1U;
    if (root) { root->width=width; root->height=height; }
    if (field) { field->x=(int)UI_TEXT_FIELD_X; field->y=(int)fy; field->width=UI_TEXT_FIELD_WIDTH<field_avail?UI_TEXT_FIELD_WIDTH:field_avail; field->height=UI_TEXT_FIELD_HEIGHT; }
    if (button) { button->x=(int)UI_BUTTON_X; button->y=(int)by; button->width=UI_BUTTON_WIDTH<button_avail?UI_BUTTON_WIDTH:button_avail; button->height=UI_BUTTON_HEIGHT; }
}

static struct nova_theme_state g_app_theme={2U,0U};
static void draw_widget_ui(struct app_buffer*buf){
 update_widget_layout(buf->width,buf->height);struct nova_widget_node*field=nova_widget_find(&g_widgets,APP_WIDGET_TEXT_FIELD),*button=nova_widget_find(&g_widgets,APP_WIDGET_BUTTON);
 int focused=g_app.focus_surface==SURFACE_A;struct nova_ui_palette p=nova_ui_palette_for(g_app_theme.choice);struct nova_ui_canvas c={(unsigned char*)buf->map,buf->width,buf->height,buf->stride};
 if(field&&field->width>8&&field->height>6){
  unsigned int edge=focused&&g_widgets.focused_id==APP_WIDGET_TEXT_FIELD?p.accent:p.line;
  nova_ui_round(c,field->x,field->y,(int)field->width,(int)field->height,4,edge);nova_ui_round(c,field->x+1,field->y+1,(int)field->width-2,(int)field->height-2,3,p.paper);
  if(nova_textedit_has_selection(&g_text)){
   unsigned int ss,se;nova_textedit_selection_bounds(&g_text,&ss,&se);unsigned int left=text_prefix_width(ss),right=text_prefix_width(se),limit=field->width-12;
   if(right>limit)right=limit;if(left<right)nova_ui_round(c,field->x+6+(int)left,field->y+2,(int)(right-left),(int)field->height-4,1,p.selection);
  }
  nova_ui_text(c,field->x+6,field->y+12,NOVA_UI_CAPTION,1,p.ink,g_text.bytes,field->x+3,field->y+2,field->width-6,field->height-4);
  if(focused&&g_widgets.focused_id==APP_WIDGET_TEXT_FIELD){unsigned int x=text_prefix_width(g_text.caret);if(x<field->width-10)nova_ui_round(c,field->x+6+(int)x,field->y+3,1,(int)field->height-6,0,p.accent);}
 }
 if(button){nova_ui_round(c,button->x,button->y,(int)button->width,(int)button->height,7,p.accent);
  (void)draw_text(buf,button->x+14,button->y+6,1U,0x00ffffffU,"Increment");
  if(focused&&g_widgets.focused_id==APP_WIDGET_BUTTON)nova_ui_round(c,button->x+(int)button->width-9,button->y+6,3,3,1,0x00ffffffU);}
}
static void render_buffer(struct app_buffer*buf){
 struct nova_ui_palette p=nova_ui_palette_for(g_app_theme.choice);fill_buffer(buf,p.paper);struct nova_ui_canvas c={(unsigned char*)buf->map,buf->width,buf->height,buf->stride};
 nova_ui_round(c,0,0,(int)buf->width,28,0,p.toolbar);
 if(buf->tile_x+TILE_SIZE>buf->width)buf->tile_x=buf->width>TILE_SIZE+12U?buf->width-TILE_SIZE-12U:8U;
 if(buf->tile_y+TILE_SIZE>buf->height)buf->tile_y=buf->height>TILE_SIZE+12U?buf->height-TILE_SIZE-12U:8U;
 nova_ui_app_icon(c,(int)buf->tile_x,(int)buf->tile_y,32,APP_CLIENT_ID);
 if (buf->surface_id==SURFACE_A) {char counter_text[]="Counter: 0";counter_text[9]=(char)('0'+g_app.counter%10U);
  nova_ui_text(c,96,64,NOVA_UI_HEADING,1,p.ink,"Interaction",0,28,buf->width,buf->height-28);
  nova_ui_text(c,96,84,NOVA_UI_CAPTION,1,p.muted,"Native example",0,28,buf->width,buf->height-28);
  nova_ui_text(c,24,126,NOVA_UI_BODY,1,p.ink,counter_text,0,28,buf->width,buf->height-28);draw_widget_ui(buf);
 }else{
  nova_ui_text(c,24,130,NOVA_UI_HEADING,1,p.ink,"Surface preview",0,28,buf->width,buf->height-28);
  nova_ui_text(c,24,153,NOVA_UI_BODY,1,p.muted,"Independent native process",0,28,buf->width,buf->height-28);
 }
 g_app.text_redraws++;
}

static int init_buffer(struct app_buffer *buf, unsigned int id, unsigned int surface_id, unsigned int width, unsigned int height,
                       unsigned int background, unsigned int accent, unsigned int header,
                       unsigned int tile_x, unsigned int tile_y) {
    nova_zero(buf,sizeof(*buf)); buf->fd=-1; buf->id=id; buf->surface_id=surface_id; buf->width=width; buf->height=height;
    if (width>NOVA_SURFACE_MAX_BUFFER_BYTES/4U) return 0;
    buf->stride=width*4U; buf->bytes=(unsigned long long)buf->stride*height;
    if (!buf->bytes || buf->bytes>NOVA_SURFACE_MAX_BUFFER_BYTES) return 0;
    long fd=nova_memfd_create("nova-toplevel-buffer",NOVA_MFD_CLOEXEC); if (fd<0) return 0;
    if (nova_ftruncate((int)fd,(nova_off_t)buf->bytes)<0) { (void)nova_close((int)fd); return 0; }
    void *map=nova_mmap((void *)0,(nova_size_t)buf->bytes,NOVA_PROT_READ | NOVA_PROT_WRITE,NOVA_MAP_SHARED,(int)fd,0);
    if (pointer_is_error(map)) { (void)nova_close((int)fd); return 0; }
    buf->fd=(int)fd; buf->map=map; buf->background=background; buf->accent=accent; buf->header=header;
    buf->tile_x=tile_x; buf->tile_y=tile_y; render_buffer(buf); return 1;
}

static void destroy_local_buffer(struct app_buffer *buf) {
    if (!buf) return;
    if (buf->map && !pointer_is_error(buf->map) && buf->bytes) (void)nova_munmap(buf->map,(nova_size_t)buf->bytes);
    if (buf->fd>=0) (void)nova_close(buf->fd);
    nova_zero(buf,sizeof(*buf)); buf->fd=-1;
}

static void init_message(struct nova_surface_message *m, unsigned int opcode, unsigned int surface_id, unsigned int sequence) {
    nova_zero(m,sizeof(*m)); m->magic=NOVA_SURFACE_PROTOCOL_MAGIC; m->version=NOVA_SURFACE_PROTOCOL_VERSION;
    m->opcode=opcode; m->size=sizeof(*m); m->client_id=APP_CLIENT_ID; m->surface_id=surface_id; m->sequence=sequence;
}

static int send_packet(int fd, const struct nova_surface_message *m) {
    long n=nova_write(fd,m,sizeof(*m)); if ((unsigned long)n!=sizeof(*m)) return 0;
    g_app.messages_sent++; if (m->opcode==NOVA_SURFACE_OP_COMMIT) g_app.commits++; return 1;
}

static int send_packet_with_fd(int fd, const struct nova_surface_message *m, int send_fd) {
    struct nova_iovec iov; struct nova_msghdr msg;
    union { unsigned char bytes[NOVA_CMSG_SPACE(sizeof(int))]; struct nova_cmsghdr align; } control;
    nova_zero(&iov,sizeof(iov)); nova_zero(&msg,sizeof(msg)); nova_zero(&control,sizeof(control));
    iov.iov_base=(void *)m; iov.iov_len=sizeof(*m); msg.msg_iov=&iov; msg.msg_iovlen=1;
    msg.msg_control=control.bytes; msg.msg_controllen=sizeof(control.bytes);
    struct nova_cmsghdr *cmsg=(struct nova_cmsghdr *)control.bytes;
    cmsg->cmsg_len=NOVA_CMSG_LEN(sizeof(int)); cmsg->cmsg_level=NOVA_SOL_SOCKET; cmsg->cmsg_type=NOVA_SCM_RIGHTS;
    nova_copy(control.bytes+NOVA_CMSG_DATA_OFFSET,&send_fd,sizeof(send_fd));
    long n=nova_sendmsg(fd,&msg,0); if ((unsigned long)n!=sizeof(*m)) return 0;
    g_app.messages_sent++; g_app.buffers_created++; return 1;
}

static int send_simple(int fd, unsigned int opcode, unsigned int surface_id, unsigned int sequence,
                       int x, int y, unsigned int w, unsigned int h, unsigned int z, unsigned int buffer_id,
                       int dx, int dy, unsigned int dw, unsigned int dh) {
    struct nova_surface_message m; init_message(&m,opcode,surface_id,sequence);
    if(opcode==NOVA_SURFACE_OP_HELLO)m.input_flags=NOVA_SURFACE_FEATURE_THEME;
    m.x=x; m.y=y; m.width=w; m.height=h; m.z_index=z; m.buffer_id=buffer_id;
    m.pixel_format=NOVA_PIXEL_FORMAT_XRGB8888; m.stride=w?w*4U:0U;
    m.damage_x=dx; m.damage_y=dy; m.damage_width=dw; m.damage_height=dh; return send_packet(fd,&m);
}

static int register_buffer(int fd, const struct app_buffer *buf, unsigned int sequence) {
    struct nova_surface_message m; init_message(&m,NOVA_SURFACE_OP_CREATE_BUFFER,0U,sequence);
    m.buffer_id=buf->id; m.width=buf->width; m.height=buf->height; m.pixel_format=NOVA_PIXEL_FORMAT_XRGB8888;
    m.stride=buf->stride; m.buffer_bytes=buf->bytes; return send_packet_with_fd(fd,&m,buf->fd);
}

static int send_role(int fd, unsigned int surface, unsigned int sequence) {
    struct nova_surface_message m; init_message(&m,NOVA_SURFACE_OP_SET_ROLE,surface,sequence); m.role=NOVA_SURFACE_ROLE_TOPLEVEL; return send_packet(fd,&m);
}

static int send_title(int fd, unsigned int surface, unsigned int sequence, const char *title) {
    struct nova_surface_message m; init_message(&m,NOVA_SURFACE_OP_SET_TITLE,surface,sequence);
    unsigned long n=nova_strlen(title); if (n>=NOVA_SURFACE_TITLE_BYTES) n=NOVA_SURFACE_TITLE_BYTES-1U;
    for (unsigned long i=0;i<n;i++) m.title[i]=title[i]; m.title[n]=0; return send_packet(fd,&m);
}

static int send_limits(int fd, unsigned int surface, unsigned int sequence,
                       unsigned int min_w, unsigned int min_h, unsigned int max_w, unsigned int max_h) {
    struct nova_surface_message m; init_message(&m,NOVA_SURFACE_OP_SET_SIZE_LIMITS,surface,sequence);
    m.min_width=min_w; m.min_height=min_h; m.max_width=max_w; m.max_height=max_h; return send_packet(fd,&m);
}

static int send_input_flags(int fd, unsigned int surface, unsigned int sequence, unsigned int flags) {
    struct nova_surface_message m; init_message(&m,NOVA_SURFACE_OP_SET_INPUT_FLAGS,surface,sequence);
    m.input_flags=flags; return send_packet(fd,&m);
}

static int send_clipboard_set(int fd, unsigned int surface, unsigned int sequence, const char *bytes, unsigned int length) {
    if (length>=NOVA_SURFACE_CLIPBOARD_BYTES) return 0;
    struct nova_surface_message m; init_message(&m,NOVA_SURFACE_OP_SET_CLIPBOARD,surface,sequence);
    m.clipboard_length=length;
    for (unsigned int i=0;i<length;i++) m.clipboard[i]=bytes[i];
    return send_packet(fd,&m);
}

static int send_clipboard_get(int fd, unsigned int surface, unsigned int sequence) {
    struct nova_surface_message m; init_message(&m,NOVA_SURFACE_OP_GET_CLIPBOARD,surface,sequence);
    return send_packet(fd,&m);
}

static int send_ack_configure(int fd, unsigned int surface, unsigned int sequence, unsigned int serial) {
    struct nova_surface_message m; init_message(&m,NOVA_SURFACE_OP_ACK_CONFIGURE,surface,sequence); m.configure_serial=serial;
    if (!send_packet(fd,&m)) return 0; g_app.configure_acks++; return 1;
}

static int send_destroy_buffer(int fd, unsigned int buffer_id, unsigned int sequence) {
    struct nova_surface_message m; init_message(&m,NOVA_SURFACE_OP_DESTROY_BUFFER,0U,sequence); m.buffer_id=buffer_id;
    if (!send_packet(fd,&m)) return 0; g_app.buffers_destroyed++; return 1;
}

static int configure_surface(int fd, unsigned int id, unsigned int *seq, int x, int y,
                             unsigned int w, unsigned int h, unsigned int z, unsigned int buffer_id,
                             const char *title, unsigned int min_w, unsigned int min_h,
                             unsigned int max_w, unsigned int max_h, unsigned int input_flags) {
    if (!send_simple(fd,NOVA_SURFACE_OP_CREATE_SURFACE,id,(*seq)++,x,y,w,h,z,0U,0,0,0,0)) return 0;
    if (!send_role(fd,id,(*seq)++)) return 0;
    if (!send_title(fd,id,(*seq)++,title)) return 0;
    if (!send_limits(fd,id,(*seq)++,min_w,min_h,max_w,max_h)) return 0;
    if (!send_input_flags(fd,id,(*seq)++,input_flags)) return 0;
    if (!send_simple(fd,NOVA_SURFACE_OP_SET_POSITION,id,(*seq)++,x,y,w,h,z,0U,0,0,0,0)) return 0;
    if (!send_simple(fd,NOVA_SURFACE_OP_ATTACH_BUFFER,id,(*seq)++,x,y,w,h,z,buffer_id,0,0,0,0)) return 0;
    if (!send_simple(fd,NOVA_SURFACE_OP_DAMAGE,id,(*seq)++,0,0,w,h,z,buffer_id,0,0,w,h)) return 0;
    if (!send_simple(fd,NOVA_SURFACE_OP_COMMIT,id,(*seq)++,x,y,w,h,z,buffer_id,0,0,0,0)) return 0;
    return 1;
}

static struct app_buffer *buffer_for_surface(unsigned int surface_id, struct app_buffer *a, struct app_buffer *b) {
    if (surface_id==SURFACE_A && !g_app.surface_a_closed) return a;
    if (surface_id==SURFACE_B && !g_app.surface_b_closed) return b;
    return (struct app_buffer *)0;
}

static void sync_buffer_state(unsigned int surface_id, const struct app_buffer *buf) {
    if (surface_id==SURFACE_A) {
        g_app.surface_a_width=buf->width; g_app.surface_a_height=buf->height; g_app.tile_a_x=buf->tile_x; g_app.buffer_a_id=buf->id;
    } else if (surface_id==SURFACE_B) {
        g_app.surface_b_width=buf->width; g_app.surface_b_height=buf->height; g_app.tile_b_x=buf->tile_x; g_app.buffer_b_id=buf->id;
    }
}

static int replace_buffer_for_configure(int fd, unsigned int *seq, const struct nova_surface_event *ev,
                                        struct app_buffer *a, struct app_buffer *b) {
    struct app_buffer *old=buffer_for_surface(ev->surface_id,a,b); if (!old || !ev->width || !ev->height) return 0;
    struct app_buffer fresh; nova_zero(&fresh,sizeof(fresh)); fresh.fd=-1;
    unsigned int new_id=g_app.next_buffer_id++; if (g_app.next_buffer_id<10U) g_app.next_buffer_id=10U;
    if (!init_buffer(&fresh,new_id,ev->surface_id,ev->width,ev->height,old->background,old->accent,old->header,old->tile_x,old->tile_y)) return 0;
    if (!register_buffer(fd,&fresh,(*seq)++)) { destroy_local_buffer(&fresh); return 0; }
    if (!send_ack_configure(fd,ev->surface_id,(*seq)++,ev->configure_serial)) { destroy_local_buffer(&fresh); return 0; }
    if (!send_simple(fd,NOVA_SURFACE_OP_ATTACH_BUFFER,ev->surface_id,(*seq)++,0,0,fresh.width,fresh.height,0U,fresh.id,0,0,0,0) ||
        !send_simple(fd,NOVA_SURFACE_OP_DAMAGE,ev->surface_id,(*seq)++,0,0,fresh.width,fresh.height,0U,fresh.id,0,0,fresh.width,fresh.height) ||
        !send_simple(fd,NOVA_SURFACE_OP_COMMIT,ev->surface_id,(*seq)++,0,0,fresh.width,fresh.height,0U,fresh.id,0,0,0,0)) {
        destroy_local_buffer(&fresh); return 0;
    }
    unsigned int old_id=old->id;
    if (!send_destroy_buffer(fd,old_id,(*seq)++)) { destroy_local_buffer(&fresh); return 0; }
    destroy_local_buffer(old); nova_copy(old,&fresh,sizeof(fresh));
    g_app.resize_buffer_replacements++; g_app.layout_generation++; sync_buffer_state(ev->surface_id,old);
    if (ev->surface_id==SURFACE_A) { g_app.surface_a_x=ev->x; g_app.surface_a_y=ev->y; }
    else { g_app.surface_b_x=ev->x; g_app.surface_b_y=ev->y; }
    nova_puts("[nova-demo-app] configure_ack surface="); nova_put_u64(ev->surface_id);
    nova_puts(" serial="); nova_put_u64(ev->configure_serial); nova_puts(" size="); nova_put_u64(ev->width);
    nova_puts("x"); nova_put_u64(ev->height); nova_putln(" buffer_replaced=yes");
    write_state("CONNECTED"); return 1;
}

static int update_focused_buffer(int fd, unsigned int *seq, struct app_buffer *a, struct app_buffer *b) {
    struct app_buffer *buf=buffer_for_surface(g_app.focus_surface,a,b); if (!buf) return 1;
    unsigned int old_x=buf->tile_x;
    if (g_app.focus_surface==SURFACE_A) buf->tile_x=buf->tile_x==48U?112U:48U;
    else buf->tile_x=buf->tile_x==80U?144U:80U;
    render_buffer(buf);
    unsigned int min_x=old_x<buf->tile_x?old_x:buf->tile_x;
    unsigned int max_x=old_x>buf->tile_x?old_x:buf->tile_x;
    unsigned int damage_w=(max_x-min_x)+TILE_SIZE;
    if (!send_simple(fd,NOVA_SURFACE_OP_DAMAGE,g_app.focus_surface,(*seq)++,0,0,buf->width,buf->height,0U,buf->id,
                     (int)min_x,(int)buf->tile_y,damage_w,TILE_SIZE) ||
        !send_simple(fd,NOVA_SURFACE_OP_COMMIT,g_app.focus_surface,(*seq)++,0,0,buf->width,buf->height,0U,buf->id,0,0,0,0)) return 0;
    g_app.buffer_updates++; sync_buffer_state(g_app.focus_surface,buf); write_state("CONNECTED"); return 1;
}

static int commit_surface_a_ui(int fd, unsigned int *seq, struct app_buffer *a) {
    if (!a || g_app.surface_a_closed) return 1;
    render_buffer(a);
    if (!send_simple(fd,NOVA_SURFACE_OP_DAMAGE,SURFACE_A,(*seq)++,0,0,a->width,a->height,0U,a->id,
                     0,0,a->width,a->height) ||
        !send_simple(fd,NOVA_SURFACE_OP_COMMIT,SURFACE_A,(*seq)++,0,0,a->width,a->height,0U,a->id,0,0,0,0)) return 0;
    g_app.buffer_updates++; g_app.layout_generation++; sync_buffer_state(SURFACE_A,a); write_state("CONNECTED"); return 1;
}

static int redraw_counter_ui(int fd, unsigned int *seq, struct app_buffer *a) {
    if (!a || g_app.surface_a_closed) return 1;
    g_app.counter=(g_app.counter%9U)+1U;
    return commit_surface_a_ui(fd,seq,a);
}

static unsigned int keycode_to_ascii(unsigned int code) {
    switch (code) {
        case NOVA_KEY_A: return 'A'; case NOVA_KEY_B: return 'B'; case NOVA_KEY_C: return 'C';
        case NOVA_KEY_D: return 'D'; case NOVA_KEY_E: return 'E'; case NOVA_KEY_F: return 'F';
        case NOVA_KEY_G: return 'G'; case NOVA_KEY_H: return 'H'; case NOVA_KEY_I: return 'I';
        case NOVA_KEY_J: return 'J'; case NOVA_KEY_K: return 'K'; case NOVA_KEY_L: return 'L';
        case NOVA_KEY_M: return 'M'; case NOVA_KEY_N: return 'N'; case NOVA_KEY_O: return 'O';
        case NOVA_KEY_P: return 'P'; case NOVA_KEY_Q: return 'Q'; case NOVA_KEY_R: return 'R';
        case NOVA_KEY_S: return 'S'; case NOVA_KEY_T: return 'T'; case NOVA_KEY_U: return 'U';
        case NOVA_KEY_V: return 'V'; case NOVA_KEY_W: return 'W'; case NOVA_KEY_X: return 'X';
        case NOVA_KEY_Y: return 'Y'; case NOVA_KEY_Z: return 'Z'; case NOVA_KEY_SPACE: return ' ';
        default: return 0U;
    }
}

static int set_widget_focus(unsigned int id) {
    if (g_widgets.focused_id==id) return 0;
    if (!nova_widget_set_focus(&g_widgets,id)) return 0;
    g_app.widget_focus_changes++; return 1;
}

static int handle_widget_key(int fd, unsigned int *seq, const struct nova_surface_event *ev,
                             struct app_buffer *a) {
    if (!ev || ev->value!=1 || ev->surface_id!=SURFACE_A || g_app.focus_surface!=SURFACE_A) return 1;
    if (ev->code==NOVA_KEY_TAB && !(ev->modifiers&NOVA_INPUT_MOD_CTRL)) {
        unsigned int old=g_widgets.focused_id;
        (void)nova_widget_focus_next(&g_widgets);
        if (g_widgets.focused_id!=old) g_app.widget_focus_changes++;
        g_app.tab_navigations++;
        if (!commit_surface_a_ui(fd,seq,a)) return 0;
        nova_puts("[nova-demo-app] key=TAB action=widget-focus-next id="); nova_put_u64(g_widgets.focused_id); nova_putln("");
        return 1;
    }
    if (g_widgets.focused_id==APP_WIDGET_BUTTON) {
        if (ev->code==NOVA_KEY_ENTER || ev->code==NOVA_KEY_SPACE) {
            if (!redraw_counter_ui(fd,seq,a)) return 0;
            g_app.keyboard_activations++;
            nova_putln("[nova-demo-app] key=ACTIVATE action=focused-button-counter-redraw");
        }
        return 1;
    }
    if (g_widgets.focused_id!=APP_WIDGET_TEXT_FIELD) return 1;

    int changed=0, edit=0;
    if (ev->modifiers&NOVA_INPUT_MOD_CTRL) {
        if (ev->code==NOVA_KEY_A) {
            nova_textedit_select_all(&g_text); g_app.selection_changes++; changed=1;
            nova_putln("[nova-demo-app] shortcut=CTRL+A action=select-all");
        } else if (ev->code==NOVA_KEY_C || ev->code==NOVA_KEY_X) {
            char clip[NOVA_SURFACE_CLIPBOARD_BYTES];
            unsigned int n=nova_textedit_copy_selection(&g_text,clip,sizeof(clip));
            if (n) {
                if (!send_clipboard_set(fd,SURFACE_A,(*seq)++,clip,n)) return 0;
                g_app.clipboard_last_length=n;
                if (ev->code==NOVA_KEY_C) { g_app.clipboard_copies++; nova_putln("[nova-demo-app] shortcut=CTRL+C action=clipboard-copy"); }
                else {
                    g_app.clipboard_cuts++;
                    if (nova_textedit_delete_selection(&g_text)) { g_app.text_edits++; g_app.selection_changes++; edit=1; changed=1; }
                    nova_putln("[nova-demo-app] shortcut=CTRL+X action=clipboard-cut");
                }
            }
        } else if (ev->code==NOVA_KEY_V) {
            if (!send_clipboard_get(fd,SURFACE_A,(*seq)++)) return 0;
            g_app.clipboard_paste_requests++;
            nova_putln("[nova-demo-app] shortcut=CTRL+V action=clipboard-request");
            write_state("CONNECTED"); return 1;
        }
    } else {
        unsigned int ascii=keycode_to_ascii(ev->code);
        if (ascii) { int had_selection=nova_textedit_has_selection(&g_text); changed=nova_textedit_insert_ascii(&g_text,ascii); if (changed) { g_app.text_inserts++; edit=1; if (had_selection) g_app.selection_changes++; } }
        else if (ev->code==NOVA_KEY_BACKSPACE) { int had_selection=nova_textedit_has_selection(&g_text); changed=nova_textedit_backspace(&g_text); if (changed) { g_app.text_backspaces++; edit=1; if (had_selection) g_app.selection_changes++; } }
        else if (ev->code==NOVA_KEY_DELETE) { int had_selection=nova_textedit_has_selection(&g_text); changed=nova_textedit_delete(&g_text); if (changed) { g_app.text_deletes++; edit=1; if (had_selection) g_app.selection_changes++; } }
        else if (ev->code==NOVA_KEY_LEFT) {
            changed=(ev->modifiers&NOVA_INPUT_MOD_SHIFT)?nova_textedit_move_left_select(&g_text):nova_textedit_move_left(&g_text);
            if (changed) { g_app.caret_moves++; if (ev->modifiers&NOVA_INPUT_MOD_SHIFT) g_app.selection_changes++; }
        } else if (ev->code==NOVA_KEY_RIGHT) {
            changed=(ev->modifiers&NOVA_INPUT_MOD_SHIFT)?nova_textedit_move_right_select(&g_text):nova_textedit_move_right(&g_text);
            if (changed) { g_app.caret_moves++; if (ev->modifiers&NOVA_INPUT_MOD_SHIFT) g_app.selection_changes++; }
        } else if (ev->code==NOVA_KEY_HOME) {
            if (g_text.caret || nova_textedit_has_selection(&g_text)) { nova_textedit_move_home(&g_text); changed=1; g_app.caret_moves++; }
        } else if (ev->code==NOVA_KEY_END) {
            if (g_text.caret!=g_text.length || nova_textedit_has_selection(&g_text)) { nova_textedit_move_end(&g_text); changed=1; g_app.caret_moves++; }
        }
    }
    if (!changed) { write_state("CONNECTED"); return 1; }
    if (edit) g_app.text_edits++;
    if (!commit_surface_a_ui(fd,seq,a)) return 0;
    nova_puts("[nova-demo-app] text_edit value="); nova_puts(g_text.bytes);
    nova_puts(" caret="); nova_put_u64(g_text.caret); nova_putln("");
    return 1;
}

static int handle_clipboard_event(int fd, unsigned int *seq, const struct nova_surface_event *ev, struct app_buffer *a) {
    if (!ev || ev->surface_id!=SURFACE_A || ev->clipboard_length>=NOVA_SURFACE_CLIPBOARD_BYTES) return 1;
    g_app.clipboard_events++; g_app.clipboard_last_length=ev->clipboard_length;
    if (!ev->clipboard_length) { write_state("CONNECTED"); return 1; }
    if (!nova_textedit_insert_ascii_bytes(&g_text,ev->clipboard,ev->clipboard_length)) { write_state("CONNECTED"); return 1; }
    g_app.clipboard_pastes++; g_app.text_edits++; g_app.selection_changes++;
    if (!commit_surface_a_ui(fd,seq,a)) return 0;
    nova_puts("[nova-demo-app] clipboard=PASTE length="); nova_put_u64(ev->clipboard_length); nova_putln("");
    return 1;
}

static int handle_widget_pointer(int fd, unsigned int *seq, const struct nova_surface_event *ev,
                                 struct app_buffer *a) {
    if (!ev || ev->surface_id!=SURFACE_A || ev->code!=NOVA_BTN_LEFT || ev->value!=1 || !a) return 1;
    int local_x=ev->cursor_x-g_app.surface_a_x, local_y=ev->cursor_y-g_app.surface_a_y;
    unsigned int hit=nova_widget_hit_test(&g_widgets,local_x,local_y);
    if (!hit) return 1;
    (void)set_widget_focus(hit);
    if (hit==APP_WIDGET_TEXT_FIELD) {
        nova_textedit_move_end(&g_text); g_app.text_field_clicks++;
        if (!commit_surface_a_ui(fd,seq,a)) return 0;
        nova_putln("[nova-demo-app] pointer=TEXT_FIELD action=focus-caret-end");
    } else if (hit==APP_WIDGET_BUTTON) {
        if (!redraw_counter_ui(fd,seq,a)) return 0;
        g_app.button_activations++;
        nova_putln("[nova-demo-app] pointer=BUTTON action=counter-increment-text-redraw");
    }
    return 1;
}

static int close_surface(int fd, unsigned int *seq, unsigned int surface_id, struct app_buffer *a, struct app_buffer *b) {
    struct app_buffer *buf=buffer_for_surface(surface_id,a,b); if (!buf) return 1;
    if (!send_simple(fd,NOVA_SURFACE_OP_DESTROY_SURFACE,surface_id,(*seq)++,0,0,0,0,0U,0U,0,0,0,0)) return 0;
    if (!send_destroy_buffer(fd,buf->id,(*seq)++)) return 0;
    destroy_local_buffer(buf); g_app.surfaces_closed++;
    if (surface_id==SURFACE_A) { g_app.surface_a_closed=1; g_app.buffer_a_id=0U; }
    else { g_app.surface_b_closed=1; g_app.buffer_b_id=0U; }
    if (g_app.focus_surface==surface_id) g_app.focus_surface=0U;
    nova_puts("[nova-demo-app] close_request surface="); nova_put_u64(surface_id); nova_putln(" action=destroy-surface-buffer");
    write_state("CONNECTED"); return 1;
}

static int open_window_set(int fd, unsigned int *seq, struct app_buffer *a, struct app_buffer *b, int initial) {
    if (!fd || !seq || !a || !b) return 0;
    if (!initial && (!g_app.surface_a_closed || !g_app.surface_b_closed)) return 1;
    unsigned int a_id=initial?BUFFER_A:g_app.next_buffer_id++;
    unsigned int b_id=initial?BUFFER_B:g_app.next_buffer_id++;
    if (g_app.next_buffer_id<10U) g_app.next_buffer_id=10U;
    g_app.surface_a_closed=0; g_app.surface_b_closed=0; g_app.focus_surface=0U;
    g_app.surface_a_x=APP_PRIMARY_X; g_app.surface_a_y=APP_PRIMARY_Y;
    g_app.surface_b_x=APP_SECONDARY_X; g_app.surface_b_y=APP_SECONDARY_Y;
    g_app.surface_a_width=SURFACE_A_WIDTH; g_app.surface_a_height=SURFACE_A_HEIGHT;
    g_app.surface_b_width=SURFACE_B_WIDTH; g_app.surface_b_height=SURFACE_B_HEIGHT;
    init_widget_tree(SURFACE_A_WIDTH,SURFACE_A_HEIGHT);
    if (!init_buffer(a,a_id,SURFACE_A,SURFACE_A_WIDTH,SURFACE_A_HEIGHT,BUFFER_A_BG,BUFFER_A_ACCENT,BUFFER_A_HEADER,48U,TILE_Y_A)) return 0;
    if (!init_buffer(b,b_id,SURFACE_B,SURFACE_B_WIDTH,SURFACE_B_HEIGHT,BUFFER_B_BG,BUFFER_B_ACCENT,BUFFER_B_HEADER,80U,TILE_Y_B)) { destroy_local_buffer(a); return 0; }
    sync_buffer_state(SURFACE_A,a); sync_buffer_state(SURFACE_B,b);
    if (!register_buffer(fd,a,(*seq)++) || !register_buffer(fd,b,(*seq)++)) return 0;
    if (!configure_surface(fd,SURFACE_A,seq,g_app.surface_a_x,g_app.surface_a_y,a->width,a->height,20U,a->id,
                           APP_TITLE_A,220U,160U,520U,420U,NOVA_SURFACE_INPUT_CAPTURE_TAB|NOVA_SURFACE_INPUT_TEXT_ASCII|NOVA_SURFACE_INPUT_CLIPBOARD)) return 0;
    if (!configure_surface(fd,SURFACE_B,seq,g_app.surface_b_x,g_app.surface_b_y,b->width,b->height,30U,b->id,
                           APP_TITLE_B,200U,140U,480U,360U,0U)) return 0;
    if (!initial) {
        g_app.window_reopens++; g_app.layout_generation++;
        nova_puts("[nova-demo-app] desktop_launch=REOPEN client="); nova_put_u64(APP_CLIENT_ID);
        nova_puts(" buffers="); nova_put_u64(a->id); nova_puts(","); nova_put_u64(b->id); nova_putln("");
    }
    write_state("CONNECTED");
    return 1;
}

static int run_connection(int fd) {
    unsigned int seq=1U; struct app_buffer a,b;
    nova_zero(&a,sizeof(a)); nova_zero(&b,sizeof(b)); a.fd=-1; b.fd=-1;
    g_app_theme.choice=2;g_app_theme.generation=0;
    g_app.next_buffer_id=10U; g_app.counter=1U; g_app.layout_generation=1UL;
    nova_textedit_init(&g_text,APP_INITIAL_TEXT);
    if (!send_simple(fd,NOVA_SURFACE_OP_HELLO,0U,seq++,0,0,0,0,0,0,0,0,0,0)) goto fail;
    if (!open_window_set(fd,&seq,&a,&b,1)) goto fail;
    nova_putln(APP_CONNECTED_LOG);
    nova_puts("[nova-demo-app] desktop_launcher=READY client="); nova_put_u64(APP_CLIENT_ID); nova_putln(" mode=resident-window-reopen");
    write_state("CONNECTED");
    for (;;) {
        struct nova_pollfd pfd; nova_zero(&pfd,sizeof(pfd)); pfd.fd=fd; pfd.events=NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP;
        struct nova_timespec timeout={1,0}; long rc=nova_poll(&pfd,1,&timeout);
        if (rc<0 && rc!=-NOVA_EINTR) goto fail; if (pfd.revents&(NOVA_POLLERR|NOVA_POLLHUP)) goto fail;
        if (pfd.revents&NOVA_POLLIN) {
            struct nova_surface_event ev; long n=nova_read(fd,&ev,sizeof(ev)); if (n<=0) goto fail;
            if ((unsigned long)n!=sizeof(ev) || ev.magic!=NOVA_SURFACE_PROTOCOL_MAGIC ||
                ev.version!=NOVA_SURFACE_PROTOCOL_VERSION || ev.size!=sizeof(ev)) continue;
            if(ev.type==NOVA_SURFACE_EVENT_THEME){
                unsigned int old=g_app_theme.choice;if(!nova_theme_accept(&g_app_theme,&ev,APP_CLIENT_ID,SURFACE_A,SURFACE_B)||old==g_app_theme.choice)continue;
                struct app_buffer*buffers[2]={&a,&b};unsigned int ids[2]={SURFACE_A,SURFACE_B};
                for(unsigned int i=0;i<2;i++)if(!(i?g_app.surface_b_closed:g_app.surface_a_closed)){
                    struct app_buffer*buf=buffers[i];render_buffer(buf);
                    if(!send_simple(fd,NOVA_SURFACE_OP_DAMAGE,ids[i],seq++,0,0,buf->width,buf->height,0,buf->id,0,0,buf->width,buf->height)||!send_simple(fd,NOVA_SURFACE_OP_COMMIT,ids[i],seq++,0,0,0,0,0,buf->id,0,0,0,0))goto fail;
                }
            } else if (ev.type==NOVA_SURFACE_EVENT_FOCUS) {
                unsigned int old_focus=g_app.focus_surface;
                g_app.focus_events++; if (ev.value) g_app.focus_surface=ev.surface_id;
                else if (g_app.focus_surface==ev.surface_id) g_app.focus_surface=0U;
                if ((old_focus==SURFACE_A || g_app.focus_surface==SURFACE_A) && !g_app.surface_a_closed) {
                    if (!commit_surface_a_ui(fd,&seq,&a)) goto fail;
                }
            } else if (ev.type==NOVA_SURFACE_EVENT_KEY) {
                g_app.key_events++;
                if (g_app.focus_surface==SURFACE_A) {
                    if (!handle_widget_key(fd,&seq,&ev,&a)) goto fail;
                } else if (ev.value==1 && ev.code==NOVA_KEY_RIGHT) {
                    if (!update_focused_buffer(fd,&seq,&a,&b)) goto fail;
                    nova_putln("[nova-demo-app] key=RIGHT action=buffer-redraw-damage-commit");
                }
            } else if (ev.type==NOVA_SURFACE_EVENT_POINTER) {
                g_app.pointer_events++;
                if (!handle_widget_pointer(fd,&seq,&ev,&a)) goto fail;
            } else if (ev.type==NOVA_SURFACE_EVENT_CLIPBOARD) {
                if (!handle_clipboard_event(fd,&seq,&ev,&a)) goto fail;
            } else if (ev.type==NOVA_SURFACE_EVENT_LAUNCH) {
                g_app.launch_events++;
                if (!open_window_set(fd,&seq,&a,&b,0)) goto fail;
            } else if (ev.type==NOVA_SURFACE_EVENT_CONFIGURE) {
                g_app.configure_events++;
                if (!replace_buffer_for_configure(fd,&seq,&ev,&a,&b)) goto fail;
            } else if (ev.type==NOVA_SURFACE_EVENT_CLOSE_REQUEST) {
                g_app.close_requests++;
                if (!close_surface(fd,&seq,ev.surface_id,&a,&b)) goto fail;
                if (g_app.surface_a_closed && g_app.surface_b_closed) {
                    nova_putln(APP_GRACEFUL_EXIT_LOG);
                    write_state("EXITING");
                    destroy_local_buffer(&b); destroy_local_buffer(&a);
                    return 1;
                }
            }
            /* Passive motion changes diagnostic counters, not widget state.
             * Keep action/focus/configure snapshots immediate; idle heartbeat
             * publishes the final counter values after motion stops. */
            static unsigned long long passive_last;
            struct nova_timespec clock; nova_zero(&clock,sizeof(clock));
            nova_clock_gettime(1,&clock);
            unsigned long long now=(unsigned long long)clock.tv_sec*1000000000ULL+clock.tv_nsec;
            if (ev.type!=NOVA_SURFACE_EVENT_POINTER || (ev.code!=NOVA_REL_X && ev.code!=NOVA_REL_Y) || now-passive_last>=250000000ULL) {
                write_state("CONNECTED");passive_last=now;
            }
        } else {
            if (!send_simple(fd,NOVA_SURFACE_OP_HEARTBEAT,0U,seq++,0,0,0,0,0,0,0,0,0,0)) goto fail;
            write_state("CONNECTED");
        }
    }
fail:
    destroy_local_buffer(&b); destroy_local_buffer(&a); return 0;
}

int nova_main(void) {
    struct nova_timespec retry={0,250000000L}; nova_zero(&g_app,sizeof(g_app));
    nova_putln(APP_START_LOG);
    for (;;) {
        struct nova_sockaddr_un addr; fill_addr(&addr); long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET,0);
        if (fd>=0) {
            int graceful_exit=0;
            if (nova_connect((int)fd,&addr,sizeof(addr))==0) graceful_exit=run_connection((int)fd);
            (void)nova_close((int)fd);
            if (graceful_exit) {
                write_state("STOPPED");
                nova_putln(APP_STOPPED_LOG);
                return 0;
            }
        }
        g_app.reconnects++; write_state("RECONNECTING"); (void)nova_nanosleep(&retry,(struct nova_timespec *)0);
    }
    return 0;
}
