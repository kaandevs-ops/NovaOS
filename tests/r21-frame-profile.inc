/* Identical profiling workload for R19/R20/R21; no DRM/FPS claims. */
 const unsigned int count=121U;double samples[121],ordered[121],copy_samples[121],parts[4][41];
 for(unsigned int i=0;i<20;i++)draw_scene(pixels,1600,900,6400,i);
 unsigned long long cpu0=nova_render_clock(2),wall0=nova_render_clock(1);
 for(unsigned int i=0;i<count;i++){unsigned long long t=nova_render_clock(1);draw_scene(pixels,1600,900,6400,i);ordered[i]=samples[i]=(double)(nova_render_clock(1)-t)/1000000.0;}
 unsigned long long elapsed=nova_render_clock(1)-wall0,cpu=nova_render_clock(2)-cpu0;qsort(samples,count,sizeof(double),compare_double);
 double copy_total=0;for(unsigned int i=0;i<count;i++){unsigned long long t=nova_render_clock(1);for(unsigned int j=0;j<NOVA_SURFACE_MAX_CLIENT_BUFFERS;j++)if(g_client_buffers[j].used)assert(r17_refresh_client_buffer(&g_client_buffers[j]));copy_samples[i]=(double)(nova_render_clock(1)-t)/1000000.0;copy_total+=copy_samples[i];}qsort(copy_samples,count,sizeof(double),compare_double);
 unsigned int old_controls=g_r7_controls_visible;
 for(unsigned int part=0;part<4;part++){
  for(unsigned int i=0;i<41;i++){unsigned long long t=nova_render_clock(1);
   if(part==0)r3_background((unsigned char*)pixels,1600,900,6400,0);
   else if(part==1)draw_client_surfaces((unsigned char*)pixels,1600,900,6400);
   else if(part==2){r3_panel((unsigned char*)pixels,1600,900,6400,g_scene.panel_height);draw_desktop_taskbar((unsigned char*)pixels,1600,900,6400);nova_r2_dock((unsigned char*)pixels,1600,900,6400);}
   else{g_r7_controls_visible=1;r7_controls((unsigned char*)pixels,1600,900,6400);}
   parts[part][i]=(double)(nova_render_clock(1)-t)/1000000.0;
  }qsort(parts[part],41,sizeof(double),compare_double);
 }g_r7_controls_visible=old_controls;
 struct rusage usage;assert(getrusage(RUSAGE_SELF,&usage)==0);unsigned long long mapped=0;for(unsigned int i=0;i<NOVA_SURFACE_MAX_CLIENT_BUFFERS;i++)if(g_client_buffers[i].used)mapped+=g_client_buffers[i].bytes;
 FILE*perf=fopen(argv[4],"w");assert(perf);fprintf(perf,"{\"scope\":\"WSL CPU native renderer 1600x900 five real surfaces, 20 warmup, 121 measured; NOT DRM/FPS\",\"samples\":121,\"p50_ms\":%.6f,\"p95_ms\":%.6f,\"max_ms\":%.6f,\"elapsed_ms\":%.3f,\"cpu_ms\":%.3f,\"max_rss_kib\":%ld,\"mapped_client_bytes\":%llu,\"snapshot_copy_total_ms\":%.3f,\"snapshot_copy_p95_ms\":%.6f,\"parts_p50_ms\":{\"wallpaper\":%.6f,\"client_composition\":%.6f,\"shell_dock\":%.6f,\"settings_surface\":%.6f},\"parts_p95_ms\":{\"wallpaper\":%.6f,\"client_composition\":%.6f,\"shell_dock\":%.6f,\"settings_surface\":%.6f},\"frame_times_ordered_ms\":[",samples[60],samples[114],samples[120],elapsed/1000000.0,cpu/1000000.0,usage.ru_maxrss,mapped,copy_total,copy_samples[114],parts[0][20],parts[1][20],parts[2][20],parts[3][20],parts[0][38],parts[1][38],parts[2][38],parts[3][38]);
 for(unsigned int i=0;i<count;i++){fprintf(perf,"%s%.6f",i?",":"",ordered[i]);}fprintf(perf,"]}\n");fclose(perf);
