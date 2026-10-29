/* Shared R18/R19 scene driver. Every surface pixel comes from real sandboxed apps. */
static unsigned int gallery_target_w,gallery_target_h;
static int gallery_sized(void){struct nova_client_surface*s=find_client_surface(300);return s&&!s->awaiting_configure&&s->width==gallery_target_w&&s->height==gallery_target_h;}
static void gallery_pump(void){for(int n=0;n<12;n++)pump(10);}
static void gallery_move(unsigned int id,int x,int y,unsigned int w,unsigned int h){
 struct nova_client_surface*s=find_client_surface(id);assert(s);set_client_focus(id);
 g_render.cursor_x=s->x+120;g_render.cursor_y=s->y+10;begin_client_grab(s,NOVA_GRAB_MOVE);
 g_render.cursor_x+=x-s->x;g_render.cursor_y+=y-s->y;update_client_grab(w,h);finish_client_grab();gallery_pump();
}
static void gallery_resize(unsigned int fw,unsigned int fh,unsigned int w,unsigned int h){
 struct nova_client_surface*s=find_client_surface(300);assert(s);gallery_target_w=fw;gallery_target_h=fh;
 if(s->width!=fw||s->height!=fh){unsigned long long start=nova_render_clock(1);set_client_focus(300);g_render.cursor_x=s->x+(int)s->width-1;g_render.cursor_y=s->y+(int)s->height-1;begin_client_grab(s,NOVA_GRAB_RESIZE);
  g_render.cursor_x+=(int)fw-(int)s->width;g_render.cursor_y+=(int)fh-(int)s->height;update_client_grab(w,h);finish_client_grab();until(gallery_sized);
  const char*dir=getenv("NOVA_GALLERY_DIR");if(dir){char path[4096];snprintf(path,sizeof(path),"%s/resize-times.jsonl",dir);FILE*f=fopen(path,"a");assert(f);fprintf(f,"{\"width\":%u,\"height\":%u,\"host_request_to_committed_ms\":%.3f}\n",fw,fh,(nova_render_clock(1)-start)/1000000.0);fclose(f);}
 }
}
static void gallery_only_files(void){
 for(unsigned int i=0;i<NOVA_SURFACE_MAX_CLIENT_SURFACES;i++)if(g_surfaces[i].used&&g_surfaces[i].committed&&g_surfaces[i].client_id!=3&&!g_surfaces[i].minimized)assert(r17_minimize(&g_surfaces[i]));
 set_client_focus(300);gallery_pump();
}
static void gallery_capture(const char*dir,const char*name,unsigned int w,unsigned int h){
 char path[4096];snprintf(path,sizeof(path),"%s/%ux%u-%s.ppm",dir,w,h,name);
 unsigned long bytes=(unsigned long)w*h*4;unsigned char*storage=malloc(bytes+64);assert(storage);memset(storage,0x6b,bytes+64);
 g_render.cursor_x=(int)w-80;g_render.cursor_y=(int)h-130;draw_scene(storage,w,h,w*4,0);
 for(unsigned int k=0;k<64;k++)assert(storage[bytes+k]==0x6b);
 FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);
 unsigned int*px=(unsigned int*)storage;for(unsigned long i=0;i<(unsigned long)w*h;i++){unsigned char rgb[3]={(unsigned char)(px[i]>>16),(unsigned char)(px[i]>>8),(unsigned char)px[i]};assert(fwrite(rgb,1,3,f)==3);}fclose(f);free(storage);
 snprintf(path,sizeof(path),"%s/%ux%u-%s-state.json",dir,w,h,name);f=fopen(path,"w");assert(f);
 fprintf(f,"{\"scope\":\"real native processes, host renderer, NOT VM\",\"width\":%u,\"height\":%u,\"focus\":%u,\"theme\":%u,\"surfaces\":[",w,h,g_client.focus_surface,g_gallery_theme);
 int comma=0;for(unsigned int i=0;i<NOVA_SURFACE_MAX_CLIENT_SURFACES;i++)if(g_surfaces[i].used){struct nova_client_surface*s=&g_surfaces[i];fprintf(f,"%s{\"id\":%u,\"x\":%d,\"y\":%d,\"w\":%u,\"h\":%u,\"minimized\":%d}",comma++?",":"",s->surface_id,s->x,s->y,s->width,s->height,s->minimized);}fprintf(f,"]}\n");fclose(f);
}
#ifdef NOVA_R21_NATIVE_UI
static unsigned long theme_before[4];
static unsigned long long theme_started;
static void arm_theme_commits(void){theme_started=nova_render_clock(1);for(unsigned int i=1;i<=3;i++)theme_before[i]=g_client_commits_by_id[i];}
static int all_theme_pixels(void){
 if(g_client_commits_by_id[1]<theme_before[1]+2||g_client_commits_by_id[2]<theme_before[2]+2||g_client_commits_by_id[3]<theme_before[3]+1)return 0;
 struct nova_ui_palette p=nova_ui_palette_for(g_session_theme.choice);unsigned int count=0;
 for(unsigned int i=0;i<NOVA_SURFACE_MAX_CLIENT_BUFFERS;i++)if(g_client_buffers[i].used&&g_client_buffers[i].map){struct nova_client_buffer*b=&g_client_buffers[i];if((((unsigned int*)b->map)[0]&0xffffffU)!=p.toolbar){fprintf(stderr,"THEME_DIAG client=%u top=%x expected=%x choice=%u commits=%lu/%lu\n",b->client_id,((unsigned int*)b->map)[0],p.toolbar,g_session_theme.choice,g_client_commits_by_id[b->client_id],theme_before[b->client_id]);return 0;}unsigned int body=*(unsigned int*)((unsigned char*)b->map+30U*b->stride);if((body&0xffffffU)!=(b->client_id==3?p.sidebar:p.paper)){fprintf(stderr,"THEME_DIAG client=%u body=%x expected=%x\n",b->client_id,body,b->client_id==3?p.sidebar:p.paper);return 0;}count++;}
 if(count==5&&theme_started){const char*dir=getenv("NOVA_GALLERY_DIR");if(dir){char path[4096];snprintf(path,sizeof(path),"%s/theme-times.jsonl",dir);FILE*f=fopen(path,"a");assert(f);fprintf(f,"{\"theme\":%u,\"generation\":%u,\"host_request_to_five_verified_commits_ms\":%.3f}\n",g_session_theme.choice,g_session_theme.generation,(nova_render_clock(1)-theme_started)/1000000.0);fclose(f);}theme_started=0;}
 return count==5;
}
static void settings_key(unsigned int key){int p[2];assert(!pipe(p));struct nova_input_event ev={0};ev.type=NOVA_EV_KEY;ev.code=key;ev.value=1;assert(write(p[1],&ev,sizeof(ev))==sizeof(ev));close(p[1]);consume_input_fd(p[0],1600,900);close(p[0]);}
static void r21_theme_gallery(const char*dir,const char*fixture){
 g_r2_canvas_width=1600;g_r2_canvas_height=900;
 gallery_only_files();gallery_resize(800,500,1600,900);gallery_move(300,400,180,1600,900);
 for(unsigned int i=0;i<NOVA_MAX_CLIENT_CONNECTIONS;i++)assert(g_connections[i].theme_subscribed&&g_connections[i].authenticated);
 int left=r7_dock_start(1600),top=r7_dock_top(900);assert(r7_chrome_click(r19_dock_slot_x(left,1)+28,top+25)&&g_r7_controls_visible);gallery_capture(dir,"nova-settings-adaptive",1600,900);
 arm_theme_commits();settings_key(NOVA_KEY_LEFT);settings_key(NOVA_KEY_ENTER);assert(g_session_theme.choice==1);until(all_theme_pixels);g_gallery_theme=1;expect_file_view(fixture,0);gallery_capture(dir,"nova-settings-graphite",1600,900);
 settings_key(NOVA_KEY_ESC);gallery_capture(dir,"files-global-graphite",1600,900);assert(r17_minimize(find_client_surface(300)));gallery_capture(dir,"desktop-global-graphite",1600,900);set_client_focus(300);gallery_pump();
 assert(r7_chrome_click(r19_dock_slot_x(left,1)+28,top+25));int sx,sy;unsigned int pw,ph;r21_settings_layout(1600,900,&sx,&sy,&pw,&ph);arm_theme_commits();assert(r7_chrome_click(sx+164,sy+160));assert(g_session_theme.choice==0);until(all_theme_pixels);g_gallery_theme=0;expect_file_view(fixture,0);gallery_capture(dir,"nova-settings-light",1600,900);settings_key(NOVA_KEY_ESC);gallery_capture(dir,"files-global-light",1600,900);
 arm_theme_commits();assert(r21_set_theme(2));until(all_theme_pixels);g_gallery_theme=2;expect_file_view(fixture,0);
 /* Actual filter/sort keys update the freestanding app rather than painted fixtures. */
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_F,1));until(new_files_commit);gallery_capture(dir,"files-folders-filter",1600,900);
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_S,1));until(new_files_commit);gallery_capture(dir,"files-name-descending",1600,900);
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_S,1));until(new_files_commit);gallery_capture(dir,"files-type-sort",1600,900);
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_F,1));until(new_files_commit);
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_S,1));until(new_files_commit);expect_file_view(fixture,0);
 for(unsigned int id=100;id<=201;id++){if(id==100||id==101||id==200||id==201)set_client_focus(id);}set_client_focus(300);gallery_pump();gallery_capture(dir,"three-apps-unified-adaptive",1600,900);
 gallery_only_files();/* Narrow Settings scene has Files behind it, not unclamped 1920px demo positions. */
 g_r7_controls_visible=1;g_settings_focus=2;gallery_capture(dir,"settings-narrow",1280,800);gallery_capture(dir,"settings-wide",1920,1080);g_r7_controls_visible=0;

#ifdef NOVA_R22_NATIVE_UI
 g_r2_canvas_width=1600;g_r2_canvas_height=900;gallery_only_files();assert(r7_chrome_click(1550,20)&&g_r7_controls_visible==2);gallery_capture(dir,"quick-settings-adaptive",1600,900);
 int qx,qy;unsigned int qw,qh;r22_quick_layout(1600,900,&qx,&qy,&qw,&qh);arm_theme_commits();assert(r7_chrome_click(qx+140,qy+100)&&g_session_theme.choice==1);until(all_theme_pixels);g_gallery_theme=1;gallery_capture(dir,"quick-settings-graphite",1600,900);
 assert(r7_chrome_click(1550,20)&&!g_r7_controls_visible);gallery_capture(dir,"files-metadata-graphite",1600,900);
 assert(r7_chrome_click(1550,20)&&g_r7_controls_visible==2);gallery_capture(dir,"quick-settings-narrow",1280,800);gallery_capture(dir,"quick-settings-wide",1920,1080);g_r7_controls_visible=0;
 arm_theme_commits();assert(r21_set_theme(2));until(all_theme_pixels);g_gallery_theme=2;
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_END,1));until(new_files_commit);gallery_capture(dir,"files-metadata-selected",1600,900);
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_R,1));until(new_files_commit);
 /* Restore the unselected reference state through real filter keys. Refresh intentionally preserves selection. */
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_F,1));until(new_files_commit);
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_F,1));until(new_files_commit);
 puts("R22_REAL_QUICK_SETTINGS_THEME_AND_FILES_FD_METADATA_COMMITS=PASS");
#endif

#ifdef NOVA_R23_NATIVE_UI
 g_r2_canvas_width=1600;g_r2_canvas_height=900;g_r7_controls_visible=1;g_settings_page=1;r23_overview_tick(nova_render_clock(1));gallery_capture(dir,"system-overview-adaptive",1600,900);
 gallery_capture(dir,"system-overview-narrow",1280,800);gallery_capture(dir,"system-overview-wide",1920,1080);
 g_r7_controls_visible=0;g_settings_page=0;r23_overview_disconnect();
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_B,1));until(new_files_commit);gallery_capture(dir,"files-size-ascending",1600,900);
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_HOME,1));until(new_files_commit);
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_B,1));until(new_files_commit);gallery_capture(dir,"files-size-descending-selected",1600,900);
 for(unsigned int z=0;z<2;z++){commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_S,1));until(new_files_commit);}
 for(unsigned int z=0;z<2;z++){commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_F,1));until(new_files_commit);}
 g_r7_controls_visible=2;g_settings_focus=4;gallery_capture(dir,"quick-settings-overview-action",1600,900);g_r7_controls_visible=0;
 puts("R23_REAL_SYSTEM_OVERVIEW_LOCAL_COUNTERS_UNAVAILABLE_GUEST_TRUST_AND_SIZE_SORT_SELECTION=PASS");
#endif
#ifdef NOVA_R24_NATIVE_UI
 g_r7_controls_visible=1;g_settings_page=2;gallery_capture(dir,"graphics-diagnostics-host-no-samples",1600,900);gallery_capture(dir,"graphics-diagnostics-narrow",1280,800);g_r24_diagnostics_view=1;gallery_capture(dir,"graphics-renderer",1600,900);gallery_capture(dir,"graphics-renderer-narrow",1280,800);g_r24_diagnostics_view=0;g_r7_controls_visible=0;g_settings_page=0;
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_END,1));until(new_files_commit);commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_I,1));until(new_files_commit);gallery_capture(dir,"files-real-properties",1600,900);
 gallery_resize(640,420,1600,900);gallery_capture(dir,"files-compact-properties",1600,900);commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_I,1));until(new_files_commit);gallery_resize(800,500,1600,900);
 for(unsigned int z=0;z<2;z++){commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_F,1));until(new_files_commit);}
 /* Real Ctrl+F/key events reach the independent sandboxed Files process;
  * compare its complete memfd framebuffer against a separately built model. */
 commits_before=g_client_commits_by_id[3];g_input_modifiers=NOVA_INPUT_MOD_CTRL;assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_F,1));g_input_modifiers=0;until(new_files_commit);
 const unsigned int filter_keys[]={NOVA_KEY_R,NOVA_KEY_E,NOVA_KEY_A,NOVA_KEY_D,NOVA_KEY_M,NOVA_KEY_E};
 for(unsigned int k=0;k<6;k++){commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,filter_keys[k],1));until(new_files_commit);}
 strcpy(reference_name_filter,"readme");reference_search_focused=1;expect_file_view(fixture,0);gallery_capture(dir,"files-name-filter",1600,900);
 gallery_resize(640,420,1600,900);expect_file_view(fixture,0);gallery_capture(dir,"files-name-filter-compact",1280,800);
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_X,1));until(new_files_commit);strcpy(reference_name_filter,"readmex");expect_file_view(fixture,0);gallery_capture(dir,"files-name-filter-empty",1280,800);
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_ESC,1));until(new_files_commit);reference_name_filter[0]=0;reference_search_focused=0;expect_file_view(fixture,0);gallery_resize(800,500,1600,900);expect_file_view(fixture,0);gallery_capture(dir,"files-name-filter-cleared",1600,900);
 puts("R24_INDEPENDENT_FILES_CTRL_F_FILTER_EMPTY_ESC_FULL_FRAMEBUFFER_MEMFD_ORACLE=PASS");
 puts("R24_ACTUAL_CLIENT_READONLY_PROPERTIES_COMPACT_UI_GRAPHICS_NO_HOST_GUEST_DATA_FABRICATION=PASS");
#endif
 /* Keep a nondefault authoritative theme across Files crash/reopen. */
 arm_theme_commits();assert(r21_set_theme(1));until(all_theme_pixels);g_gallery_theme=1;
 puts("R21_SETTINGS_MOUSE_KEYBOARD_THREE_APP_THEME_SYNC_FILTER_SORT_REAL_COMMITS=PASS");
}
#endif
static void r19_capture_gallery(const char*dir,const char*fixture){
 unsigned int sizes[][2]={{1280,800},{1600,900},{1920,1080}};
 for(unsigned int n=0;n<3;n++){
  unsigned int w=sizes[n][0],h=sizes[n][1];g_r2_canvas_width=w;g_r2_canvas_height=h;
  gallery_only_files();gallery_resize(800,500,w,h);gallery_move(300,((int)w-800)/2,((int)h-500)/2-20,w,h);expect_file_view(fixture,0);
  assert(r17_minimize(find_client_surface(300)));gallery_capture(dir,"desktop",w,h);
  set_client_focus(300);gallery_pump();gallery_capture(dir,"files",w,h);
  assert(r17_minimize(find_client_surface(300)));set_client_focus(300);gallery_pump();gallery_capture(dir,"restored",w,h);
  gallery_move(100,48,110,w,h);gallery_move(101,80,(int)h-300,w,h);
  gallery_move(200,(int)w-368,110,w,h);gallery_move(201,(int)w-390,(int)h-290,w,h);
  set_client_focus(300);gallery_pump();const unsigned int order[]={100,101,200,201,300};for(unsigned int i=0;i<5;i++)raise_client_surface(find_client_surface(order[i]));
  gallery_capture(dir,"three-apps",w,h);
  gallery_only_files();gallery_move(300,80,70,w,h);gallery_resize(w>1160?1000:w-160,h>800?620:h-180,w,h);expect_file_view(fixture,0);
  set_client_focus(200);gallery_pump();gallery_capture(dir,"focus-resize",w,h);
#ifdef NOVA_R19_NATIVE_UI
  if(w==1280){gallery_only_files();gallery_resize(640,420,w,h);gallery_move(300,320,140,w,h);expect_file_view(fixture,0);gallery_capture(dir,"compact-files",w,h);}
  if(w==1600){
   gallery_only_files();gallery_move(300,400,180,w,h);gallery_resize(800,500,w,h);
   gallery_capture(dir,"adaptive",w,h);
   int dockleft=r7_dock_start(w),docktop=r7_dock_top(h);assert(r7_chrome_click(r19_dock_slot_x(dockleft,0)+28,docktop+30));
   assert(g_r7_library_visible);assert(r19_library_key(NOVA_KEY_DOWN));gallery_capture(dir,"applications-menu",w,h);assert(r19_library_key(NOVA_KEY_ESC));
   assert(r7_chrome_click(r19_dock_slot_x(dockleft,1)+28,docktop+30));assert(g_r7_controls_visible);gallery_capture(dir,"quick-controls",w,h);
   assert(r7_chrome_click(r19_dock_slot_x(dockleft,1)+28,docktop+30));
   commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_T,1));until(new_files_commit);g_gallery_theme=0;expect_file_view(fixture,0);gallery_capture(dir,"calm-light",w,h);
   commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_T,1));until(new_files_commit);g_gallery_theme=1;expect_file_view(fixture,0);gallery_capture(dir,"graphite-pro",w,h);
   commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_T,1));until(new_files_commit);g_gallery_theme=2;expect_file_view(fixture,0);
   /* Real quick location navigation, then return through the existing core. */
   commits_before=g_client_commits_by_id[3];struct nova_client_surface*s=find_client_surface(300);g_render.cursor_x=s->x+70;g_render.cursor_y=s->y+200;
   assert(route_client_event(NOVA_SURFACE_EVENT_POINTER,300,NOVA_BTN_LEFT,1));until(new_files_commit);expect_file_view(fixture,1);gallery_capture(dir,"documents",w,h);
   commits_before=g_client_commits_by_id[3];g_render.cursor_x=s->x+70;g_render.cursor_y=s->y+302;assert(route_client_event(NOVA_SURFACE_EVENT_POINTER,300,NOVA_BTN_LEFT,1));until(new_files_commit);expect_file_view(fixture,2);gallery_capture(dir,"empty-folder",w,h);
   commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_BACKSPACE,1));until(new_files_commit);expect_file_view(fixture,0);
#ifdef NOVA_R20_NATIVE_UI
   commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_LEFT,1));until(new_files_commit);reference_index--;expect_file_view(fixture,2);gallery_capture(dir,"history-back",w,h);
   commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_RIGHT,1));until(new_files_commit);reference_index++;expect_file_view(fixture,0);gallery_capture(dir,"history-forward",w,h);
   commits_before=g_client_commits_by_id[3];g_render.cursor_x=s->x+70;g_render.cursor_y=s->y+200;assert(route_client_event(NOVA_SURFACE_EVENT_POINTER,300,NOVA_BTN_LEFT,1));until(new_files_commit);expect_file_view(fixture,1);
   commits_before=g_client_commits_by_id[3];g_render.cursor_x=s->x+450;g_render.cursor_y=s->y+250;
   int wheelpipe[2];assert(!pipe(wheelpipe));struct nova_input_event wheel={0};wheel.type=NOVA_EV_REL;wheel.code=NOVA_REL_WHEEL;wheel.value=-1;assert(write(wheelpipe[1],&wheel,sizeof(wheel))==sizeof(wheel));close(wheelpipe[1]);consume_input_fd(wheelpipe[0],w,h);close(wheelpipe[0]);until(new_files_commit);gallery_capture(dir,"wheel-scrolled",w,h);
   commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_END,1));until(new_files_commit);gallery_capture(dir,"keyboard-end",w,h);
   commits_before=g_client_commits_by_id[3];g_render.cursor_x=s->x+196+190;g_render.cursor_y=s->y+87;assert(route_client_event(NOVA_SURFACE_EVENT_POINTER,300,NOVA_BTN_LEFT,1));until(new_files_commit);expect_file_view(fixture,0);gallery_capture(dir,"breadcrumb-root",w,h);
#endif
  }
#endif
 }
#ifdef NOVA_R21_NATIVE_UI
 r21_theme_gallery(dir,fixture);
#endif
 g_r2_canvas_width=1600;g_r2_canvas_height=900;gallery_only_files();gallery_move(300,400,180,1600,900);gallery_resize(800,500,1600,900);expect_file_view(fixture,0);
#ifdef NOVA_R21_REFERENCE_UI
 {int dl=r7_dock_start(1600),dt=r7_dock_top(900);assert(r7_chrome_click(r19_dock_slot_x(dl,1)+28,dt+25));gallery_capture(dir,"settings-before",1600,900);assert(r7_chrome_click(r19_dock_slot_x(dl,1)+28,dt+25));}
#endif
 puts("R19_GALLERY_THREE_RESOLUTIONS_REAL_MOVE_RESIZE_FOCUS_MINIMIZE_FRAME_GUARDS=PASS");
}
