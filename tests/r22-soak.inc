/* Host-only bounded stability exercise. Never installed into initramfs. */
static unsigned int r22_fds(int pid){char path[64];snprintf(path,sizeof(path),"/proc/%d/fd",pid);DIR*d=opendir(path);assert(d);unsigned int n=0;while(readdir(d))n++;closedir(d);return n;}
static unsigned long r22_rss(int pid){char path[64],line[256];snprintf(path,sizeof(path),"/proc/%d/status",pid);FILE*f=fopen(path,"r");assert(f);unsigned long value=0;while(fgets(line,sizeof(line),f))if(sscanf(line,"VmRSS: %lu kB",&value)==1)break;fclose(f);return value;}
static void r22_soak(const pid_t kids[3]){const char*setting=getenv("NOVA_R22_SOAK_CYCLES");if(!setting)return;unsigned long cycles=strtoul(setting,0,10);assert(cycles>=1&&cycles<=20000);unsigned int fds[4];fds[0]=r22_fds(getpid());for(unsigned int j=0;j<3;j++)fds[j+1]=r22_fds(kids[j]);
 const char*out=getenv("NOVA_R22_SOAK_OUTPUT");assert(out);FILE*f=fopen(out,"w");assert(f);unsigned long long start=nova_render_clock(1);fprintf(f,"{\"scope\":\"bounded host real-process theme+metadata-refresh cycles; NOT guest or 24h soak\",\"cycles\":%lu,\"samples\":[",cycles);unsigned int samples=0;
 for(unsigned long i=0;i<cycles;i++){
  arm_theme_commits();assert(r21_set_theme((unsigned int)(i%3)));until(all_theme_pixels);g_gallery_theme=g_session_theme.choice;
  commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_R,1));until(new_files_commit);
  assert(client_connection_count()==3&&client_surface_count_for(1)==2&&client_surface_count_for(2)==2&&client_surface_count_for(3)==1);
  if(i%100==0||i+1==cycles){assert(r22_fds(getpid())==fds[0]+1);/* own open JSON output */for(unsigned int j=0;j<3;j++)assert(r22_fds(kids[j])==fds[j+1]);fprintf(f,"%s{\"cycle\":%lu,\"elapsed_ms\":%.3f,\"parent_rss_kib\":%lu,\"app_rss_kib\":[%lu,%lu,%lu],\"fds_unchanged\":true}",samples++?",":"",i+1,(nova_render_clock(1)-start)/1000000.0,r22_rss(getpid()),r22_rss(kids[0]),r22_rss(kids[1]),r22_rss(kids[2]));fflush(f);}
 }
 fprintf(f,"],\"elapsed_ms\":%.3f,\"status\":\"PASS\"}\n",(nova_render_clock(1)-start)/1000000.0);fclose(f);puts("R22_REAL_THREE_APPS_THEME_METADATA_REFRESH_FD_SURFACE_SOAK=PASS");
}
