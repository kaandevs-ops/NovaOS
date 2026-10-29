static int r25_has_proof(void){for(unsigned y=0;y<g_terminal.grid.rows;y++){unsigned *row=nova_term_live_row(&g_terminal.grid,y);const char *t="multi-app-proof";unsigned x=0;while(t[x]&&row[x]==(unsigned char)t[x])x++;if(!t[x])return 1;}return 0;}
static void r25_terminal_multi(void){
 unsigned before=client_connection_count();assert(before==3);unsigned focused=g_client.focus_surface;
 g_terminal.visible=1;g_terminal.focused=1;assert(nova_term_start());r25_terminal_focus();r25_terminal_layout(1600,900);
 unsigned long long end=nova_render_clock(1)+8000000000ULL;
 while(!g_terminal.ready&&nova_render_clock(1)<end){nova_term_receive();pump(1);}assert(g_terminal.ready);nova_term_flush();
 const char *command="echo multi-app-proof\r";
 for(unsigned k=0;command[k];k++){
  unsigned found=0;for(unsigned shift=0;shift<2&&!found;shift++)for(unsigned code=1;code<58;code++)if(nova_term_key_ascii(code,shift,0)==(unsigned char)command[k]){nova_term_input(code,shift,0);found=1;break;}assert(found);
 }
 end=nova_render_clock(1)+8000000000ULL;while(!r25_has_proof()&&nova_render_clock(1)<end){nova_term_receive();nova_term_flush();pump(1);}assert(r25_has_proof());
 assert(client_connection_count()==3&&!g_client.focus_surface);
 int session_fd=g_terminal.fd;long session_pid=g_terminal.child;
 unsigned terminal_visits=0,app_visits=0;
 for(unsigned n=0;n<1000;n++){
  assert(r17_switch_window());pump(1);
  assert(g_terminal.visible&&g_terminal.fd==session_fd&&g_terminal.child==session_pid);
  assert(client_connection_count()==3);
  if(g_terminal.focused){assert(!g_client.focus_surface);terminal_visits++;}
  else{assert(g_client.focus_surface);app_visits++;}
 }
 assert(terminal_visits&&app_visits);r25_terminal_focus();for(unsigned id=1;id<=3;id++)assert(client_id_is_connected(id));
 const char *out=getenv("NOVA_R25_MULTI_OUT");if(out){unsigned *pixels=calloc(1600*900,4);assert(pixels);draw_scene(pixels,1600,900,6400,0);save_ppm(out,pixels);free(pixels);}
 nova_term_disconnect(0);g_terminal.visible=0;end=nova_render_clock(1)+4000000000ULL;while(g_terminal.child>0&&nova_render_clock(1)<end){nova_term_reap();pump(1);}assert(!g_terminal.child&&client_connection_count()==before);g_terminal.focused=0;set_client_focus(focused);
 puts("R25_REAL_TERMINAL_WITH_THREE_AUTHENTICATED_APPS_PRIVATE_NAMESPACE_PTY_AND_CLEANUP=PASS (HOST ONLY)");
}
