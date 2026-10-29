#include "../../src/runtime/nova_desktop_open.h"
#include "nova_files_surface_bridge.h"
#include "../../src/runtime/nova_theme.h"
#include "../../src/runtime/nova_input.h"
/* Existing key-code transport is ASCII-only; no Unicode input is claimed. */
static unsigned int files_filter_ascii(unsigned int code,unsigned int modifiers){
 const char*letters="qwertyuiopasdfghjklzxcvbnm";
 const unsigned int codes[]={16,17,18,19,20,21,22,23,24,25,30,31,32,33,34,35,36,37,38,44,45,46,47,48,49,50};
 for(unsigned int i=0;i<26;i++)if(code==codes[i])return (unsigned int)letters[i]-((modifiers&NOVA_INPUT_MOD_SHIFT)?32U:0U);
 if(code>=2&&code<=11){const char*plain="1234567890",*shift="!@#$%^&*()";return (unsigned int)((modifiers&NOVA_INPUT_MOD_SHIFT)?shift:plain)[code-2];}
 if(code==57)return ' ';if(code==12)return modifiers&NOVA_INPUT_MOD_SHIFT?'_':'-';if(code==52)return '.';if(code==53)return '/';return 0;
}
void nova_files_surface_bridge_init(struct nova_files_surface_bridge*b,
 struct nova_files_ui*u,unsigned int c,unsigned int s,int x,int y){
 if(!b)return;b->ui=u;b->client_id=c;b->surface_id=s;b->surface_x=x;b->surface_y=y;
 b->closed=0;b->configured=0;b->rejected=0;b->theme_generation=0;b->desktop_open_sequence=0;
}
int nova_files_surface_bridge_event(struct nova_files_surface_bridge*b,const struct nova_surface_event*e){
 if(!b||!e||!b->ui)return -2;
 if(e->magic!=NOVA_SURFACE_PROTOCOL_MAGIC||e->version!=NOVA_SURFACE_PROTOCOL_VERSION||
    e->size!=sizeof(*e)||e->client_id!=b->client_id||e->surface_id!=b->surface_id){
  b->rejected++;return -2;
 }
 if(b->closed && e->type!=NOVA_SURFACE_EVENT_LAUNCH)return 0;
 if(e->type==NOVA_SURFACE_EVENT_THEME){struct nova_theme_state state={b->ui->theme,b->theme_generation};unsigned int old=state.choice;
  if(!nova_theme_accept(&state,e,b->client_id,b->surface_id,0)){b->rejected++;return -2;}b->theme_generation=state.generation;b->ui->theme=state.choice;return old!=state.choice;
 }
 if(e->type==NOVA_SURFACE_EVENT_DESKTOP_OPEN){
  if(!nova_desktop_open_event(e)||e->sequence<=b->desktop_open_sequence){b->rejected++;return -2;}
  b->desktop_open_sequence=e->sequence;
  return nova_files_ui_open_desktop(b->ui,e->clipboard);
 }
 switch(e->type){
  case NOVA_SURFACE_EVENT_CLOSE_REQUEST:b->closed=1;return 0;
  case NOVA_SURFACE_EVENT_LAUNCH:b->closed=0;return 1;
  case NOVA_SURFACE_EVENT_CONFIGURE:
   if(!nova_files_ui_resize(b->ui,e->width,e->height))return 0;
   b->surface_x=e->x;b->surface_y=e->y;b->configured=1;return 1;
  case NOVA_SURFACE_EVENT_POINTER:
   if(e->code!=NOVA_BTN_LEFT && e->code!=NOVA_REL_X && e->code!=NOVA_REL_Y && e->code!=NOVA_REL_WHEEL)return 0;
   {
    long long x=(long long)e->cursor_x-(long long)b->surface_x;
    long long y=(long long)e->cursor_y-(long long)b->surface_y;
    if(x<0LL||y<0LL||x>=b->ui->viewport_width||y>=b->ui->viewport_height)
     return e->code==NOVA_BTN_LEFT?(e->value==0?nova_files_ui_pointer(b->ui,-1,-1,1,0):0):nova_files_ui_pointer(b->ui,-1,-1,0,0);
    if(e->code==NOVA_REL_WHEEL)return nova_files_ui_scroll(b->ui,e->value);
    return nova_files_ui_pointer(b->ui,(int)x,(int)y,e->code==NOVA_BTN_LEFT?1:0,e->value==1);
   }
  case NOVA_SURFACE_EVENT_KEY:
   if(e->value!=1)return 0;
   if((e->modifiers&NOVA_INPUT_MOD_CTRL)&&e->code==NOVA_KEY_F)return nova_files_ui_find(b->ui);
   if(b->ui->search_focused){
    if(e->modifiers&NOVA_INPUT_MOD_CTRL)return 0;
    if(e->code==NOVA_KEY_ESC)return nova_files_ui_filter_key(b->ui,27);
    if(e->code==NOVA_KEY_ENTER)return nova_files_ui_filter_key(b->ui,13);
    if(e->code==NOVA_KEY_BACKSPACE)return nova_files_ui_filter_key(b->ui,8);
    if(e->code==NOVA_KEY_LEFT||e->code==NOVA_KEY_RIGHT||e->code==NOVA_KEY_HOME||e->code==NOVA_KEY_END)return 0;
    if(!(e->modifiers&NOVA_INPUT_MOD_CTRL)){unsigned int ascii=files_filter_ascii(e->code,e->modifiers);if(ascii)return nova_files_ui_filter_key(b->ui,ascii);}
   }
   if(e->code==NOVA_KEY_ESC)return nova_files_ui_key(b->ui,27);
   if(e->code==NOVA_KEY_ENTER)return nova_files_ui_key(b->ui,13U);
   if(e->code==NOVA_KEY_BACKSPACE)return nova_files_ui_key(b->ui,8U);
   if(e->code==NOVA_KEY_LEFT)return nova_files_ui_key(b->ui,'[');
   if(e->code==NOVA_KEY_RIGHT)return nova_files_ui_key(b->ui,']');
   if(e->code==NOVA_KEY_HOME)return nova_files_ui_key(b->ui,36);
   if(e->code==NOVA_KEY_END)return nova_files_ui_key(b->ui,35);
   if(e->code==NOVA_KEY_PAGEUP)return nova_files_ui_key(b->ui,33);
   if(e->code==NOVA_KEY_PAGEDOWN)return nova_files_ui_key(b->ui,34);
   if(!(e->modifiers&~NOVA_INPUT_MOD_SHIFT)&&e->code==NOVA_KEY_N)return nova_files_ui_key(b->ui,'n');
   if(!(e->modifiers&~NOVA_INPUT_MOD_SHIFT)&&e->code==NOVA_KEY_P)return nova_files_ui_key(b->ui,'p');
   if(e->code==NOVA_KEY_I)return nova_files_ui_key(b->ui,'i');
   if(e->code==NOVA_KEY_B)return nova_files_ui_key(b->ui,'b');
   if(e->code==NOVA_KEY_F)return nova_files_ui_key(b->ui,'f');
   if(e->code==NOVA_KEY_S)return nova_files_ui_key(b->ui,'s');
   if(e->code==NOVA_KEY_T)return nova_files_ui_key(b->ui,'t');
   if(e->code==NOVA_KEY_R)return nova_files_ui_key(b->ui,'r');
   if(e->code==NOVA_KEY_J||e->code==NOVA_KEY_DOWN)return nova_files_ui_key(b->ui,'j');
   if(e->code==NOVA_KEY_K||e->code==NOVA_KEY_UP)return nova_files_ui_key(b->ui,'k');
   return 0;
  default:return 0;
 }
}
