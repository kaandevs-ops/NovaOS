#ifndef NOVA_DESKTOP_FILE_UI_H
#define NOVA_DESKTOP_FILE_UI_H
static char g_desktop_last_click_name[64];
static unsigned g_desktop_text_scroll,g_desktop_text_lines,g_desktop_history_count,g_desktop_history[160],g_desktop_has_focus;
static unsigned long long g_desktop_last_click_ns;
static int r54_desktop_busy(void){return g_desktop_file_pending||g_desktop_file_inflight;}
static int r54_desktop_request(unsigned action,const char*name,unsigned offset){
 if(r54_desktop_busy())return 0;
 struct nova_desktop_file_request r;nova_zero(&r,sizeof r);r.action=action;r.offset=offset;
 if(name){unsigned n=(unsigned)nova_strlen(name);if(n>=sizeof r.name)return 0;nova_copy(r.name,name,n+1);}
 if(!nova_desktop_file_request_valid(&r))return 0;
 if(g_terminal.fd<0&&!nova_term_start())return 0;
 nova_copy(&g_desktop_file_request,&r,sizeof r);g_desktop_file_pending=1;g_desktop_file_error=0;nova_term_flush();g_scene.visual_generation++;return 1;
}
static int r54_desktop_trash_at(int x,int y){return g_r2_canvas_width>=360&&g_r2_canvas_height>=300&&x>=(int)g_r2_canvas_width-124&&x<(int)g_r2_canvas_width-24&&y>=(int)g_r2_canvas_height-202&&y<(int)g_r2_canvas_height-112;}
static void r54_desktop_open_selected(void){
 if(g_desktop_selected==~0U||g_desktop_selected>=g_desktop_files.count||!g_desktop_selected_name[0])return;
 if(g_desktop_files.entries[g_desktop_selected].kind==2){
  g_desktop_file_error=r57_desktop_open_folder(g_desktop_selected_name)?0:5;
  g_scene.visual_generation++;return;
 }
 if(r54_desktop_request(NOVA_DESKTOP_PREVIEW,g_desktop_selected_name,0)){
  g_desktop_history_count=0;g_desktop_text_scroll=0;g_desktop_document_visible=1;g_desktop_trash_visible=0;
  nova_zero(&g_desktop_preview,sizeof g_desktop_preview);nova_copy(g_desktop_preview.name,g_desktop_selected_name,nova_strlen(g_desktop_selected_name)+1);
  g_terminal.focused=0;set_client_focus(0);
 }
}
static void r54_desktop_show_trash(void){
 if(r54_desktop_request(NOVA_DESKTOP_TRASH_LIST,0,0)){g_desktop_document_visible=0;g_desktop_trash_visible=1;g_desktop_trash_scroll=0;g_desktop_trash_selected=~0U;g_terminal.focused=0;set_client_focus(0);}
}
static void r54_desktop_trash_page(int direction){
 unsigned start=g_desktop_trash_files.start;if(direction<0){if(!start)return;start=start>NOVA_DESKTOP_TRASH_LIMIT?start-NOVA_DESKTOP_TRASH_LIMIT:0;}else{start+=g_desktop_trash_files.count;if(start>=g_desktop_trash_files.total)return;}
 (void)r54_desktop_request(NOVA_DESKTOP_TRASH_LIST,0,start);
}
static void r54_desktop_restore(void){if(g_desktop_trash_selected<g_desktop_trash_files.count)(void)r54_desktop_request(NOVA_DESKTOP_TRASH_RESTORE,g_desktop_trash_files.entries[g_desktop_trash_selected].token,0);}
static void r54_desktop_page(int direction){
 if(r54_desktop_busy()||g_desktop_preview.status)return;
 unsigned offset=g_desktop_preview.offset;
 if(direction>0){if(offset+g_desktop_preview.length>=g_desktop_preview.total||g_desktop_history_count>=160)return;
  unsigned next=offset+g_desktop_preview.length;if(!g_desktop_preview.length)return;
  if(r54_desktop_request(NOVA_DESKTOP_PREVIEW,g_desktop_preview.name,next))g_desktop_history[g_desktop_history_count++]=offset;
 }else if(g_desktop_history_count){if(r54_desktop_request(NOVA_DESKTOP_PREVIEW,g_desktop_preview.name,g_desktop_history[g_desktop_history_count-1]))g_desktop_history_count--;}
 g_desktop_text_scroll=0;
}
static void r54_desktop_layout(int*x,int*y,int*w,int*h){*w=g_r2_canvas_width>864?800:(int)g_r2_canvas_width-48;*h=g_r2_canvas_height>720?540:(int)g_r2_canvas_height-150;if(*w<280)*w=280;if(*h<160)*h=160;*x=((int)g_r2_canvas_width-*w)/2;*y=((int)g_r2_canvas_height-*h)/2-16;}
static void r54_desktop_button(struct nova_ui_canvas c,int x,int y,int w,const char*text,struct nova_ui_palette p,int disabled){
 unsigned state=disabled?NOVA_UI_DISABLED:0;if(!disabled&&g_render.cursor_x>=x&&g_render.cursor_x<x+w&&g_render.cursor_y>=y&&g_render.cursor_y<y+32)state|=NOVA_UI_HOVER;
 nova_ui_button(c,x,y,w,32,p,state);nova_ui_text(c,x+10,y+22,NOVA_UI_BODY,1,disabled?p.muted:p.ink,text,x+6,y,w-12,32);
}
static const char*r54_desktop_error(unsigned e){return e==13?"Erişim reddedildi. Dosya izinlerini kontrol edin.":e==17?"Aynı ad zaten var. Geri yükleme dosyanın üzerine yazmadı.":e==21?"Klasör gezinmesi için Files uygulamasını kullanın.":e==95?"Metin önizlemesi desteklenmiyor (ikili dosya veya 256 KiB sınırı).":e==2?"Dosya artık bu konumda değil. Masaüstünü yenileyin.":"İşlem doğrulanamadı. Yenileyerek mevcut durumu kontrol edin.";}
static void r54_desktop_text(struct nova_ui_canvas c,int x,int y,int w,int h,unsigned ink){
 char line[256];unsigned pos=0,len=0,number=0,previous=~0U;int line_width=0;const struct nova_ui_face*face=nova_ui_face_for(NOVA_UI_BODY,1);
 while(pos<g_desktop_preview.length||len){
  unsigned cp=0,n=pos<g_desktop_preview.length?nova_ui_utf8(g_desktop_preview.text+pos,g_desktop_preview.length-pos,&cp):0;
  if(cp==13){pos+=n;continue;}
  char glyph[8]={0};unsigned gn=n;if(cp==9){gn=4;for(unsigned i=0;i<4;i++)glyph[i]=' ';}else if(n)for(unsigned i=0;i<n;i++)glyph[i]=(char)g_desktop_preview.text[pos+i];
  unsigned current=n?nova_ui_glyph_index(cp==9?' ':cp):0;
  int gw=n?face->glyphs[current].advance+(previous==~0U?0:nova_ui_kerning(face,previous,current)):0;
  if(cp==9)gw+=3*(face->glyphs[current].advance+nova_ui_kerning(face,current,current));
  if(!n||cp==10||(len&&((line_width+gw+63)/64>w||len+gn>=sizeof line))){
   line[len]=0;if(number>=g_desktop_text_scroll){int yy=y+18+(int)(number-g_desktop_text_scroll)*24;if(yy<=y+h)nova_ui_text(c,x,yy,NOVA_UI_BODY,1,ink,line,x,y,w,h);}
   number++;len=0;line_width=0;previous=~0U;if(!n)break;if(cp==10)pos+=n;continue;
  }
  if(gn>=sizeof line-len)break;for(unsigned i=0;i<gn;i++)line[len++]=glyph[i];line_width+=gw;previous=current;pos+=n;
 }
 g_desktop_text_lines=number;
}
static void r54_desktop_overlay_draw(unsigned char*b,unsigned width,unsigned height,unsigned pitch){
 if(!g_desktop_document_visible&&!g_desktop_trash_visible)return;
 struct nova_ui_canvas c={b,width,height,pitch};struct nova_ui_palette p=nova_ui_palette_for(g_session_theme.choice);int x,y,w,h;r54_desktop_layout(&x,&y,&w,&h);
 r19_shadow(b,width,height,pitch,x,y,w,h,24,1);r8_surface(b,width,height,pitch,x,y,w,h,12,p.paper,p.accent);
 const char*title=g_desktop_trash_visible?"Çöp Kutusu":g_desktop_preview.name;char name[96];nova_ui_ellipsis(title,name,sizeof name,NOVA_UI_HEADING,w-90);
 nova_ui_text(c,x+20,y+31,NOVA_UI_HEADING,1,p.ink,name,x+16,y+8,w-76,32);r54_desktop_button(c,x+w-48,y+10,32,"",p,0);nova_ui_line(c,x+w-37,y+20,x+w-27,y+30,p.ink);nova_ui_line(c,x+w-27,y+20,x+w-37,y+30,p.ink);
 nova_ui_text(c,x+20,y+57,NOVA_UI_CAPTION,1,p.muted,g_desktop_trash_visible?"Geri alınabilir taşıma · Kalıcı silme yok":"Metin önizlemesi · Salt okunur · Esc: kapat",x+16,y+36,w-32,28);
 unsigned status=g_desktop_trash_visible?(g_desktop_trash_files.status?g_desktop_trash_files.status:g_desktop_file_error):(g_desktop_preview.status?g_desktop_preview.status:g_desktop_file_error);
 if(r54_desktop_busy())nova_ui_text(c,x+22,y+93,NOVA_UI_BODY,1,p.muted,"Yükleniyor…",x+16,y+70,w-32,30);
 else if(status&&!g_desktop_trash_visible){char msg[200];nova_ui_ellipsis(r54_desktop_error(status),msg,sizeof msg,NOVA_UI_BODY,w-48);nova_ui_text(c,x+24,y+105,NOVA_UI_BODY,1,p.ink,msg,x+20,y+76,w-40,60);}
 if(g_desktop_trash_visible){
  unsigned rows=h>156?(unsigned)(h-156)/36:0;
  if(!r54_desktop_busy()&&!g_desktop_trash_files.count&&!status)nova_ui_text(c,x+24,y+113,NOVA_UI_BODY,1,p.muted,"Çöp kutusu boş. Taşınan öğeler burada görünür.",x+20,y+84,w-40,70);
  for(unsigned i=0;i<rows&&i+g_desktop_trash_scroll<g_desktop_trash_files.count;i++){
   unsigned index=i+g_desktop_trash_scroll;int yy=y+76+(int)i*36;struct nova_desktop_trash_entry*e=&g_desktop_trash_files.entries[index];if(index==g_desktop_trash_selected)nova_ui_round(c,x+16,yy,w-32,34,6,p.selection);
   if(e->kind==2)nova_ui_folder(c,x+26,yy+7,22,p.accent);else nova_ui_document(c,x+28,yy+6,20,p.muted);
   nova_ui_ellipsis(e->name,name,sizeof name,NOVA_UI_BODY,w-100);nova_ui_text(c,x+62,yy+24,NOVA_UI_BODY,1,p.ink,name,x+58,yy,w-80,34);
  }
  if(status){char msg[200];nova_ui_ellipsis(r54_desktop_error(status),msg,sizeof msg,NOVA_UI_CAPTION,w-48);nova_ui_text(c,x+24,y+h-64,NOVA_UI_CAPTION,1,p.ink,msg,x+20,y+h-92,w-40,32);}
  r54_desktop_button(c,x+18,y+h-48,96,"Yenile",p,r54_desktop_busy());r54_desktop_button(c,x+w-156,y+h-48,138,"Geri yükle",p,r54_desktop_busy()||g_desktop_trash_selected>=g_desktop_trash_files.count);
  if(w>=500){r54_desktop_button(c,x+122,y+h-48,32,"<",p,r54_desktop_busy()||!g_desktop_trash_files.start);r54_desktop_button(c,x+160,y+h-48,32,">",p,r54_desktop_busy()||g_desktop_trash_files.start+g_desktop_trash_files.count>=g_desktop_trash_files.total);}
  if(g_desktop_trash_files.clipped)nova_ui_text(c,x+204,y+h-26,NOVA_UI_CAPTION,1,p.muted,"Liste sınırı: 256",x+200,y+h-48,w-360,32);
 }else{
  if(!r54_desktop_busy()&&!status){if(!g_desktop_preview.total)nova_ui_text(c,x+24,y+108,NOVA_UI_BODY,1,p.muted,"Bu metin dosyası boş.",x+20,y+80,w-40,50);else r54_desktop_text(c,x+24,y+76,w-48,h-150,p.ink);}
  r54_desktop_button(c,x+18,y+h-48,96,"Önceki",p,r54_desktop_busy()||!g_desktop_history_count);r54_desktop_button(c,x+w-118,y+h-48,100,"Sonraki",p,r54_desktop_busy()||status||g_desktop_preview.offset+g_desktop_preview.length>=g_desktop_preview.total);
  nova_ui_text(c,x+130,y+h-26,NOVA_UI_CAPTION,1,p.muted,"Yukarı/Aşağı: kaydır · PgUp/PgDn: bölüm",x+124,y+h-48,w-250,32);
 }
}
static int r54_desktop_overlay_click(int x,int y){
 if(!g_desktop_document_visible&&!g_desktop_trash_visible)return 0;int bx,by,w,h;r54_desktop_layout(&bx,&by,&w,&h);int lx=x-bx,ly=y-by;
 if(lx>=w-48&&lx<w-16&&ly>=10&&ly<42){g_desktop_document_visible=g_desktop_trash_visible=0;}
 else if(g_desktop_trash_visible){
  unsigned rows=h>156?(unsigned)(h-156)/36:0;
  if(lx>=16&&lx<w-16&&ly>=76&&ly<76+(int)rows*36){unsigned index=g_desktop_trash_scroll+(unsigned)(ly-76)/36;if(index<g_desktop_trash_files.count)g_desktop_trash_selected=index;}
  if(ly>=h-48&&ly<h-16){if(lx>=18&&lx<114)(void)r54_desktop_request(NOVA_DESKTOP_TRASH_LIST,0,0);else if(lx>=w-156&&lx<w-18)r54_desktop_restore();else if(w>=500&&lx>=122&&lx<154)r54_desktop_trash_page(-1);else if(w>=500&&lx>=160&&lx<192)r54_desktop_trash_page(1);}
 }else if(ly>=h-48&&ly<h-16){if(lx>=18&&lx<114)r54_desktop_page(-1);else if(lx>=w-118&&lx<w-18)r54_desktop_page(1);}
 g_scene.visual_generation++;return 1;
}
static int r54_desktop_overlay_key(unsigned key){
 if(!g_desktop_document_visible&&!g_desktop_trash_visible)return 0;
 if(key==NOVA_KEY_ESC){g_desktop_document_visible=g_desktop_trash_visible=0;}
 else if(g_desktop_trash_visible){
  if(key==104)r54_desktop_trash_page(-1);else if(key==109)r54_desktop_trash_page(1);else if(key==NOVA_KEY_R)(void)r54_desktop_request(NOVA_DESKTOP_TRASH_LIST,0,0);
  else if(key==NOVA_KEY_ENTER)r54_desktop_restore();
  else if(key==NOVA_KEY_DOWN||key==NOVA_KEY_UP){unsigned count=g_desktop_trash_files.count;if(count){if(g_desktop_trash_selected>=count)g_desktop_trash_selected=0;else if(key==NOVA_KEY_DOWN&&g_desktop_trash_selected+1<count)g_desktop_trash_selected++;else if(key==NOVA_KEY_UP&&g_desktop_trash_selected)g_desktop_trash_selected--;int x,y,w,h;r54_desktop_layout(&x,&y,&w,&h);unsigned rows=h>156?(unsigned)(h-156)/36:1;if(!rows)rows=1;if(g_desktop_trash_selected<g_desktop_trash_scroll)g_desktop_trash_scroll=g_desktop_trash_selected;if(g_desktop_trash_selected>=g_desktop_trash_scroll+rows)g_desktop_trash_scroll=g_desktop_trash_selected-rows+1;}}
 }else if(key==104)r54_desktop_page(-1);else if(key==109)r54_desktop_page(1);
 else if(key==NOVA_KEY_UP){if(g_desktop_text_scroll)g_desktop_text_scroll--;}
 else if(key==NOVA_KEY_DOWN){if(g_desktop_text_scroll+1<g_desktop_text_lines)g_desktop_text_scroll++;}
 else if(key==NOVA_KEY_HOME){if(r54_desktop_request(NOVA_DESKTOP_PREVIEW,g_desktop_preview.name,0)){g_desktop_history_count=0;g_desktop_text_scroll=0;}}
 g_scene.visual_generation++;return 1;
}
#endif
