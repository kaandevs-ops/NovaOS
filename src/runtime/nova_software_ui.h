#ifndef NOVA_SOFTWARE_UI_H
#define NOVA_SOFTWARE_UI_H
static char r59_query[49];
static unsigned r59_query_length,r59_search_focus;
static int r59_match(const char*s){if(!r59_query_length)return 1;for(unsigned a=0;s[a];a++){unsigned k=0;while(k<r59_query_length&&s[a+k]){unsigned x=(unsigned char)s[a+k],y=(unsigned char)r59_query[k];if(x>=65&&x<=90)x+=32;if(y>=65&&y<=90)y+=32;if(x!=y)break;k++;}if(k==r59_query_length)return 1;}return 0;}
static void r58_layout(unsigned w,unsigned h){
 if(!g_sw.w){g_sw.w=w>1000?900:(int)w-40;g_sw.h=h>760?590:(int)h-150;g_sw.x=((int)w-g_sw.w)/2;g_sw.y=58;}
 if(g_sw.maximized){g_sw.x=16;g_sw.y=46;g_sw.w=(int)w-32;g_sw.h=(int)h-142;}
 if(g_sw.w>(int)w-32)g_sw.w=(int)w-32;if(g_sw.h>(int)h-140)g_sw.h=(int)h-140;
 if(g_sw.w<320)g_sw.w=320;if(g_sw.h<260)g_sw.h=260;
 if(g_sw.x<16)g_sw.x=16;if(g_sw.x+g_sw.w>(int)w-16)g_sw.x=(int)w-16-g_sw.w;
 if(g_sw.y<46)g_sw.y=46;if(g_sw.y+g_sw.h>(int)h-94)g_sw.y=(int)h-94-g_sw.h;
}
static int r58_at(int x,int y){return g_sw.visible&&x>=g_sw.x&&x<g_sw.x+g_sw.w&&y>=g_sw.y&&y<g_sw.y+g_sw.h;}
static void r58_open(unsigned page){g_sw.visible=1;g_sw.focused=1;g_sw.page=page;g_sw.confirm=0;g_terminal.focused=0;g_r7_controls_visible=0;g_r7_library_visible=0;set_client_focus(0);if(g_sw.fd<0)(void)nova_sw_start();if(g_store.fd<0)(void)nova_store_start();if(g_sw.ready&&!g_sw.pending)(void)nova_sw_request(NOVA_SW_LIST,0);g_scene.visual_generation++;}
static unsigned r58_rows(unsigned*items){unsigned n=0;for(unsigned i=0;i<4;i++)if((g_sw.tab==0&&i!=3)||(g_sw.tab==1&&i==3)||(g_sw.tab==2&&g_sw.installed[i]==0)||(g_sw.tab==3&&(g_store.available&(1U<<i)))){if(r59_match(nova_sw_titles[i])||r59_match(nova_sw_descriptions[i]))items[n++]=i;}return n;}
static int r58_selected(void){unsigned items[4],n=r58_rows(items);if(!n)return -1;for(unsigned k=0;k<n;k++)if(items[k]==g_sw.selected)return (int)g_sw.selected;g_sw.selected=items[0];return (int)items[0];}
static void r58_app_open(unsigned index){if(index>=4)return;g_sw.selected=index;r58_open(index+1);if(!(g_sw.running&(1U<<index)))g_sw.page=index+5;}
static void r58_tick(void){
 nova_sw_flush();
 if(g_sw.ready&&!g_sw.pending&&g_sw.page>=5&&g_sw.page<=8){unsigned i=g_sw.page-5;g_sw.page=i+1;if(g_sw.installed[i]==0)(void)nova_sw_request(NOVA_SW_RUN,i);else g_sw.last_result=2;g_scene.visual_generation++;}
 if(g_sw.visible&&g_sw.page>=1&&g_sw.page<=4&&(g_sw.running&(1U<<(g_sw.page-1)))&&!g_sw.pending){unsigned i=g_sw.page-1,cols=(unsigned)(g_sw.w-40)/9,rows=(unsigned)(g_sw.h-126)/18;if(cols<20)cols=20;if(cols>120)cols=120;if(rows<4)rows=4;if(rows>48)rows=48;
  if(cols!=g_sw.app_cols[i]||rows!=g_sw.app_rows[i]){unsigned dims[]={rows,cols};if(nova_sw_request_data(NOVA_SW_RESIZE,i,dims,8)>0){g_sw.app_rows[i]=rows;g_sw.app_cols[i]=cols;nova_term_grid_resize(&g_sw.grids[i],rows,cols);g_scene.visual_generation++;}}
 }
}
static void r58_button(struct nova_ui_canvas c,struct nova_ui_palette p,int x,int y,int width,const char*label,int disabled){unsigned hover=g_render.cursor_x>=x&&g_render.cursor_x<x+width&&g_render.cursor_y>=y&&g_render.cursor_y<y+34;unsigned state=(disabled?NOVA_UI_DISABLED:0)|(hover?NOVA_UI_HOVER:0);nova_ui_button(c,x,y,width,34,p,state);nova_ui_text(c,x+12,y+23,NOVA_UI_BODY,1,disabled?p.muted:p.ink,label,x+6,y,width-12,34);}
static void r58_draw(unsigned char*b,unsigned w,unsigned h,unsigned pitch){
 if(!g_sw.visible||w<360||h<400)return;r58_layout(w,h);struct nova_ui_canvas c={b,w,h,pitch};struct nova_ui_palette p=nova_ui_palette_for(g_session_theme.choice);int x=g_sw.x,y=g_sw.y,ww=g_sw.w,hh=g_sw.h;
 r19_shadow(b,w,h,pitch,x,y,ww,hh,24,1);r8_surface(b,w,h,pitch,x,y,ww,hh,14,p.paper,p.line);
 const char*title=g_sw.page>=1&&g_sw.page<=4?nova_sw_titles[g_sw.page-1]:"Nova Software";nova_ui_text(c,x+22,y+30,NOVA_UI_HEADING,1,p.ink,title,x+16,y+6,ww-148,34);
 nova_ui_line(c,x+ww-100,y+25,x+ww-88,y+25,p.ink);nova_ui_line(c,x+ww-66,y+19,x+ww-54,y+19,p.ink);nova_ui_line(c,x+ww-66,y+19,x+ww-66,y+31,p.ink);nova_ui_line(c,x+ww-54,y+19,x+ww-54,y+31,p.ink);nova_ui_line(c,x+ww-66,y+31,x+ww-54,y+31,p.ink);nova_ui_line(c,x+ww-32,y+19,x+ww-20,y+31,p.ink);nova_ui_line(c,x+ww-32,y+31,x+ww-20,y+19,p.ink);
 if(g_sw.page){r58_button(c,p,x+18,y+48,122,"Katalog",0);nova_ui_text(c,x+156,y+72,NOVA_UI_CAPTION,1,p.muted,"Ayrı terminal uygulaması oturumu",x+148,y+48,ww-164,30);
  if(g_sw.page<=4){unsigned i=g_sw.page-1;struct nova_term_grid*g=&g_sw.grids[i];nova_ui_round(c,x+16,y+88,ww-32,hh-120,8,0x00161e29);
   for(unsigned row=0;row<g->rows&&row<(unsigned)(hh-126)/18;row++){const unsigned*line=nova_term_view_row(g,row);for(unsigned col=0;col<g->cols&&col<(unsigned)(ww-40)/9;col++)if(line[col]&&line[col]!=' ')nova_term_glyph(c,x+20+(int)col*9,y+92+(int)row*18,line[col],0x00e5efec);}
   if(g_sw.last_result==105)nova_ui_text(c,x+22,y+hh-10,NOVA_UI_CAPTION,1,p.muted,"Girdi kuyruğu dolu; bekleyip son tuşu yeniden deneyin.",x+16,y+hh-30,ww-32,28);
   else if(!(g_sw.running&(1U<<i)))nova_ui_text(c,x+22,y+hh-10,NOVA_UI_CAPTION,1,p.muted,g_sw.exit_code[i]?"Uygulama hata ile kapandı; katalogdan yeniden açabilirsiniz.":"Oturum kapandı. Kataloğa dönebilirsiniz.",x+16,y+hh-30,ww-32,28);
  }return;
 }
 const char*tabs[]={"Yerel","Oyunlar","Kurulu","İnternet"};int tabwidth=(ww-56)/4;
 for(unsigned t=0;t<4;t++){int xx=x+16+(int)t*(tabwidth+8);nova_ui_button(c,xx,y+50,tabwidth,36,p,t==g_sw.tab?NOVA_UI_FOCUSED:0);nova_ui_text(c,xx+12,y+74,NOVA_UI_BODY,1,p.ink,tabs[t],xx+6,y+50,tabwidth-12,36);}
 nova_ui_button(c,x+16,y+94,ww-(g_sw.tab==3&&ww>600?212:32),34,p,r59_search_focus?NOVA_UI_FOCUSED:0);nova_ui_text(c,x+28,y+117,NOVA_UI_BODY,1,r59_query_length?p.ink:p.muted,r59_query_length?r59_query:"Uygulama ara · Ctrl+F",x+22,y+94,ww-(g_sw.tab==3&&ww>600?224:44),34);if(g_sw.tab==3&&ww>600)r58_button(c,p,x+ww-186,y+94,170,"Tümünü indir",!g_store.ready||!g_sw.ready||g_store.pending||g_sw.pending||!g_store.available);
 unsigned items[4],count=r58_rows(items);int selected=r58_selected();unsigned capacity=(unsigned)(hh-258)/66;if(capacity<1)capacity=1;if(capacity>4)capacity=4;
 unsigned ordinal=0;for(unsigned k=0;k<count;k++)if(items[k]==g_sw.selected)ordinal=k;if(ordinal<g_sw.scroll)g_sw.scroll=ordinal;if(ordinal>=g_sw.scroll+capacity)g_sw.scroll=ordinal-capacity+1;
 for(unsigned n=0;n<capacity&&g_sw.scroll+n<count;n++){unsigned i=items[g_sw.scroll+n];int yy=y+142+(int)n*66;unsigned state=i==g_sw.selected?NOVA_UI_FOCUSED:0;if(g_render.cursor_x>=x+16&&g_render.cursor_x<x+ww-16&&g_render.cursor_y>=yy&&g_render.cursor_y<yy+60)state|=NOVA_UI_HOVER;nova_ui_button(c,x+16,yy,ww-32,60,p,state);nova_ui_dock_icon(c,x+24,yy+5,i+8);nova_ui_text(c,x+88,yy+24,NOVA_UI_HEADING,1,p.ink,nova_sw_titles[i],x+82,yy+4,ww-102,28);
  const char*s=g_sw.installed[i]==0?"Kurulu · doğrulandı":g_sw.installed[i]==3?"Kurulu değil":g_sw.installed[i]==1?"Doğrulama hatası":"Durum bekleniyor";nova_ui_text(c,x+88,yy+46,NOVA_UI_CAPTION,1,p.muted,s,x+82,yy+28,ww-102,26);
 }
 if(!count)nova_ui_text(c,x+26,y+178,NOVA_UI_BODY,1,p.muted,r59_query_length?"Aramanızla eşleşen uygulama yok.":g_sw.tab==3?"İnternet deposu yapılandırılmamış.":"Bu bölümde henüz uygulama yok.",x+20,y+146,ww-40,70);
 if(selected>=0){char text[128];nova_ui_ellipsis(nova_sw_descriptions[selected],text,sizeof text,NOVA_UI_BODY,ww-48);nova_ui_text(c,x+24,y+hh-98,NOVA_UI_BODY,1,p.ink,text,x+18,y+hh-122,ww-36,38);
  int bottom=y+hh-78;if(g_sw.confirm){nova_ui_text(c,x+24,bottom-6,NOVA_UI_CAPTION,1,p.ink,"Seçili uygulama kaldırılsın mı? Kullanıcı verileri korunur.",x+18,bottom-28,ww-36,28);r58_button(c,p,x+20,bottom,112,"Vazgeç",0);r58_button(c,p,x+144,bottom,112,"Kaldır",g_sw.pending);}
  else{int bw=ww<400?(ww-60)/3:96;int rx=20+2*(bw+10);r58_button(c,p,x+20,bottom,bw,g_sw.tab==3&&g_store.pending&&ww<=490?"İptal":g_sw.tab==3?"İndir":"Kur",!g_sw.ready||g_sw.pending||(g_sw.tab==3&&g_store.pending&&ww<=490?g_store.pending==2:g_sw.installed[selected]!=3||(g_sw.tab==3&&(!g_store.ready||g_store.pending||!(g_store.available&(1U<<selected))))));r58_button(c,p,x+30+bw,bottom,bw,"Aç",!g_sw.ready||g_sw.pending||g_sw.installed[selected]!=0);r58_button(c,p,x+rx,bottom,ww<400?bw:110,"Kaldır",!g_sw.ready||g_sw.pending||g_sw.installed[selected]==3||g_sw.installed[selected]==4||(g_sw.running&(1U<<selected)));if(ww>490)r58_button(c,p,x+ww-130,bottom,110,g_sw.tab==3&&g_store.pending?"İptal":"Yenile",g_sw.tab==3&&g_store.pending?g_store.pending==2:!g_sw.ready||g_sw.pending);}
 }
 const char*status=g_sw.error?"Uygulama servisine ulaşılamadı. Pencereyi kapatıp yeniden açın.":!g_sw.ready?"Güvenli uygulama servisi başlatılıyor…":g_sw.pending?"İşlem sürüyor…":g_sw.last_result?"İşlem tamamlanamadı; paket durumu ve çalışan oturumu kontrol edin.":"Doğrulanmış yerel katalog · İnternet deposu henüz yok";
 char download[220];if(g_sw.tab==3){nova_store_status(download);status=download;}
 char info[160];nova_ui_ellipsis(status,info,sizeof info,NOVA_UI_CAPTION,ww-36);nova_ui_text(c,x+20,y+hh-15,NOVA_UI_CAPTION,1,p.muted,info,x+16,y+hh-34,ww-32,30);
}
static int r58_click(int x,int y){
 if(!r58_at(x,y))return 0;g_sw.focused=1;g_terminal.focused=0;set_client_focus(0);int lx=x-g_sw.x,ly=y-g_sw.y;
 if(ly<46){if(lx>=g_sw.w-42){if(g_sw.page>=1&&g_sw.page<=4)g_sw.close_requested|=1U<<(g_sw.page-1);g_sw.visible=0;g_sw.focused=0;}
  else if(lx>=g_sw.w-76){g_sw.maximized^=1U;if(!g_sw.maximized){g_sw.w=900;g_sw.h=590;}r58_layout(g_r2_canvas_width,g_r2_canvas_height);}
  else if(lx>=g_sw.w-110){g_sw.visible=0;g_sw.focused=0;}else{g_sw.drag=1;g_sw.gx=x-g_sw.x;g_sw.gy=y-g_sw.y;}g_scene.visual_generation++;return 1;}
 if(g_sw.page){if(lx>=18&&lx<140&&ly>=48&&ly<82){g_sw.page=0;g_sw.confirm=0;if(!g_sw.pending)(void)nova_sw_request(NOVA_SW_LIST,0);}g_scene.visual_generation++;return 1;}
 if(g_sw.tab==3&&g_sw.w>600&&lx>=g_sw.w-186&&lx<g_sw.w-16&&ly>=94&&ly<128){(void)r59_store_install_available();g_scene.visual_generation++;return 1;}
 if(ly>=94&&ly<128){r59_search_focus=1;g_scene.visual_generation++;return 1;}r59_search_focus=0;
 if(ly>=50&&ly<86){int tabwidth=(g_sw.w-56)/4;unsigned tab=(unsigned)(lx-16)/(unsigned)(tabwidth+8);if(lx>=16&&tab<4){g_sw.tab=tab;g_sw.scroll=0;g_sw.confirm=0;}g_scene.visual_generation++;return 1;}
 unsigned items[4],count=r58_rows(items);unsigned capacity=(unsigned)(g_sw.h-258)/66;if(capacity<1)capacity=1;if(capacity>4)capacity=4;
 if(ly>=142&&ly<142+(int)capacity*66){unsigned row=(unsigned)(ly-142)/66+g_sw.scroll;if(row<count){g_sw.selected=items[row];g_sw.confirm=0;}g_scene.visual_generation++;return 1;}
 int selected=r58_selected();if(selected>=0&&ly>=g_sw.h-78&&ly<g_sw.h-44&&!g_sw.pending&&g_sw.ready){unsigned i=(unsigned)selected;int bw=g_sw.w<400?(g_sw.w-60)/3:96,ax=30+bw,rx=20+2*(bw+10);
  if(g_sw.confirm){if(lx>=20&&lx<132)g_sw.confirm=0;else if(lx>=144&&lx<256){g_sw.confirm=0;(void)nova_sw_request(NOVA_SW_REMOVE,i);}}
  else if(lx>=20&&lx<20+bw&&(g_sw.installed[i]==3||(g_sw.tab==3&&g_store.pending&&g_sw.w<=490))){if(g_sw.tab==3&&g_store.pending&&g_sw.w<=490)(void)nova_store_request(NS_CANCEL,g_store.app);else if(g_sw.tab==3)(void)nova_store_request(NS_FETCH,i);else (void)nova_sw_request(NOVA_SW_INSTALL,i);}
  else if(lx>=ax&&lx<ax+bw&&g_sw.installed[i]==0)r58_app_open(i);
  else if(lx>=rx&&lx<rx+(g_sw.w<400?bw:110)&&g_sw.installed[i]!=3&&g_sw.installed[i]!=4&&!(g_sw.running&(1U<<i)))g_sw.confirm=1;
  else if(g_sw.w>490&&lx>=g_sw.w-130&&lx<g_sw.w-20){if(g_sw.tab==3&&g_store.pending)(void)nova_store_request(NS_CANCEL,g_store.app);else (void)nova_sw_request(NOVA_SW_LIST,0);}
 }g_scene.visual_generation++;return 1;
}
static int r58_key(unsigned key,unsigned modifiers){
 if(!g_sw.visible||!g_sw.focused)return 0;
 if(!g_sw.page&&g_sw.tab==3&&key==23&&(modifiers&NOVA_INPUT_MOD_CTRL)){(void)r59_store_install_available();g_scene.visual_generation++;return 1;}
 if(!g_sw.page&&key==33&&(modifiers&NOVA_INPUT_MOD_CTRL)){r59_search_focus=1;g_scene.visual_generation++;return 1;}
 if(!g_sw.page&&r59_search_focus){if(key==NOVA_KEY_ESC){r59_query[0]=0;r59_query_length=0;r59_search_focus=0;}else if(key==NOVA_KEY_ENTER||key==NOVA_KEY_TAB)r59_search_focus=0;else if(key==14){if(r59_query_length){do{r59_query_length--;}while(r59_query_length&&((unsigned char)r59_query[r59_query_length]&192)==128);r59_query[r59_query_length]=0;}}else{unsigned cp=nova_trq_key(key,modifiers&NOVA_INPUT_MOD_SHIFT,modifiers&NOVA_INPUT_MOD_CTRL,g_terminal_altgr,g_terminal_caps);unsigned char bytes[4];unsigned n=nova_keyboard_utf8(cp,bytes);if(cp>=32&&cp!=127&&n&&r59_query_length+n<sizeof r59_query){nova_copy(r59_query+r59_query_length,bytes,n);r59_query_length+=n;r59_query[r59_query_length]=0;}}g_sw.scroll=0;g_scene.visual_generation++;return 1;}
 if(key==NOVA_KEY_ESC&&g_sw.tab==3&&g_store.pending&&!g_sw.page){if(g_store.pending==1)(void)nova_store_request(NS_CANCEL,g_store.app);g_scene.visual_generation++;return 1;}
 if(g_sw.page>=1&&g_sw.page<=4){unsigned i=g_sw.page-1;if(!(g_sw.running&(1U<<i)))return 1;
  const char*seq=key==103?"\033[A":key==108?"\033[B":key==105?"\033[D":key==106?"\033[C":key==102?"\033[H":key==107?"\033[F":key==111?"\033[3~":0;
  if(seq)(void)nova_sw_input(i,seq,(unsigned)nova_strlen(seq));else{unsigned cp=nova_trq_key(key,modifiers&NOVA_INPUT_MOD_SHIFT,modifiers&NOVA_INPUT_MOD_CTRL,g_terminal_altgr,g_terminal_caps);if(cp){unsigned char bytes[4];unsigned n=nova_keyboard_utf8(cp,bytes);(void)nova_sw_input(i,bytes,n);}}return 1;
 }
 if(key==NOVA_KEY_ESC){if(g_sw.confirm)g_sw.confirm=0;else g_sw.visible=0;}
 else if(key==NOVA_KEY_TAB){g_sw.tab=(g_sw.tab+1)%4;g_sw.scroll=0;g_sw.confirm=0;}
 else if(key==NOVA_KEY_DOWN||key==NOVA_KEY_UP){unsigned items[4],count=r58_rows(items);for(unsigned k=0;k<count;k++)if(items[k]==g_sw.selected){g_sw.selected=items[key==NOVA_KEY_DOWN?(k+1)%count:(k+count-1)%count];break;}}
 else if(key==NOVA_KEY_R&&!g_sw.pending&&g_sw.ready)(void)nova_sw_request(NOVA_SW_LIST,0);
 else if(key==NOVA_KEY_ENTER&&!g_sw.pending&&g_sw.ready){int i=r58_selected();if(i>=0){if(g_sw.confirm){g_sw.confirm=0;(void)nova_sw_request(NOVA_SW_REMOVE,(unsigned)i);}else if(g_sw.installed[i]==3){if(g_sw.tab==3)(void)nova_store_request(NS_FETCH,(unsigned)i);else (void)nova_sw_request(NOVA_SW_INSTALL,(unsigned)i);}else if(!g_sw.installed[i])r58_app_open((unsigned)i);}}
 g_scene.visual_generation++;return 1;
}
static int r58_motion(int x,int y){if(!g_sw.drag)return 0;g_sw.x=x-g_sw.gx;g_sw.y=y-g_sw.gy;r58_layout(g_r2_canvas_width,g_r2_canvas_height);g_scene.visual_generation++;return 1;}
#endif
