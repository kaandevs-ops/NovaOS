#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <execinfo.h>
#define NOVA_RENDER_TEST_REFERENCE 1
#define NOVA_R19_NATIVE_UI 1
#define NOVA_R20_NATIVE_UI 1
#define NOVA_R21_NATIVE_UI 1
#define NOVA_R22_NATIVE_UI 1
#define NOVA_R23_NATIVE_UI 1
#define NOVA_R24_NATIVE_UI 1
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
#include "nova_files_ui.h"
#include "nova_files_guest_adapter.h"
int r17_fixture_register(const char*,int);
int r17_fixture_metadata(unsigned int,char*,unsigned int);
void r17_fixture_authority(int,int);
static int clients[NOVA_MAX_CLIENT_CONNECTIONS];
static void pump(unsigned int millis){
 struct nova_pollfd fds[NOVA_MAX_CLIENT_CONNECTIONS];setup_app_pollfds(fds,0,clients);
 for(unsigned int i=0;i<NOVA_MAX_CLIENT_CONNECTIONS;i++)assert(fds[i].fd==clients[i]);
 struct nova_timespec timeout={0,(long)millis*1000000L};
 long rc=nova_poll(fds,NOVA_MAX_CLIENT_CONNECTIONS,&timeout);assert(rc>=0||rc==-NOVA_EINTR);
 consume_app_pollfds(fds,0,clients,1600,900);
}
static void until(int (*predicate)(void)){
 unsigned long long end=nova_render_clock(1)+8000000000ULL;
 while(!predicate()&&nova_render_clock(1)<end)pump(10);
 if(!predicate()){fprintf(stderr,"WAIT_FAILURE commits=%lu,%lu,%lu surfaces=%u theme=%u generation=%u\n",g_client_commits_by_id[1],g_client_commits_by_id[2],g_client_commits_by_id[3],client_surface_count(),g_session_theme.choice,g_session_theme.generation);void*trace[12];int n=backtrace(trace,12);backtrace_symbols_fd(trace,n,2);}
 assert(predicate());
}
static int five_surfaces(void){return client_surface_count()==5&&find_client_surface(300)&&find_client_surface(300)->committed;}
static int files_maximized(void){struct nova_client_surface*s=find_client_surface(300);return s&&s->width==1584&&s->height==726&&!s->awaiting_configure;}
static int files_restored(void){struct nova_client_surface*s=find_client_surface(300);return s&&s->width==800&&s->height==500&&!s->awaiting_configure;}
static int no_files(void){return !client_surface_count_for(3)&&!client_id_is_connected(3);}
static int no_clients(void){return !client_connection_count();}
static int a_position_acked(void){struct nova_client_surface*s=find_client_surface(100);return s&&s->x==1060&&s->y==662&&!s->awaiting_configure;}
static int acked(void){struct nova_client_surface*s=find_client_surface(300);return s&&!s->awaiting_configure;}
static unsigned int g_gallery_theme=2;
static void *expected_files_pixels;
static int exact_file_view(void){
 struct nova_client_surface *s=find_client_surface(300);if(!s)return 0;
 struct nova_client_buffer *b=find_client_buffer(3,s->buffer_id);if(!b||!b->map)return 0;
 const unsigned char *a=b->map,*e=expected_files_pixels;
 for(unsigned long i=0;i<b->bytes;i++)if(a[i]!=e[i])return 0;
 return 1;
}
static char reference_name_filter[64];static unsigned int reference_search_focused;
static char reference_history[16][256];static unsigned int reference_count,reference_index;
#include "../files/src/nova_files_guest_metadata.h"
static void expect_file_view(const char *fixture,int documents){
 char path[4096];snprintf(path,sizeof(path),"%s/Data/Nova/Apps/nova.files",fixture);
 int fd=open(path,O_RDONLY|O_DIRECTORY);assert(fd>=0);
 struct nova_files_model m;assert(nova_files_init(&m,fd,&nova_files_guest_ops,0)==0&&nova_files_list(&m)==0);

 struct nova_client_surface *s=find_client_surface(300);assert(s);
 struct nova_files_ui ui;nova_files_ui_init(&ui,&m);
#ifdef NOVA_R19_NATIVE_UI
 ui.theme=g_gallery_theme;
#endif
if(documents)assert(nova_files_enter(&m,documents==2?"Projects":"Documents")==0);assert(nova_files_ui_resize(&ui,s->width,s->height));
 /* Independent bounded history oracle; retain full framebuffer byte comparison. */
 if(!reference_count){strcpy(reference_history[0],"/");reference_count=1;reference_index=0;}
 if(strcmp(reference_history[reference_index],m.path)){
  unsigned int next=reference_index+1;if(next==16){memmove(reference_history,reference_history+1,15*256);next=15;}
  strcpy(reference_history[next],m.path);reference_index=next;reference_count=next+1;
 }
 memcpy(ui.history,reference_history,sizeof(reference_history));ui.history_count=reference_count;ui.history_index=reference_index;
 nova_files_ui_set_metadata(&ui,nova_files_guest_metadata);
 strcpy(ui.name_filter,reference_name_filter);ui.search_focused=reference_search_focused;
 expected_files_pixels=malloc((unsigned long)s->width*s->height*4U);assert(expected_files_pixels);
 assert(nova_files_ui_render(&ui,expected_files_pixels,s->width,s->height,s->width*4U));
 until(exact_file_view);free(expected_files_pixels);expected_files_pixels=0;nova_files_close(&m);
}
static unsigned long commits_before;
static int new_files_commit(void){return g_client_commits_by_id[3]>commits_before;}
static void save_ppm(const char *path,unsigned int *pixels){
 FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n1600 900\n255\n");
 for(unsigned int i=0;i<1600*900;i++){unsigned char rgb[3]={(unsigned char)(pixels[i]>>16),(unsigned char)(pixels[i]>>8),(unsigned char)pixels[i]};assert(fwrite(rgb,1,3,f)==3);}assert(fclose(f)==0);
}
static int compare_double(const void*a,const void*b){double x=*(const double*)a,y=*(const double*)b;return x<y?-1:x>y;}
#include "r19-gallery.inc"
#include "r22-soak.inc"
#include "r25-terminal-multi.inc"
int main(int argc,char **argv){
 assert((argc==5||argc==6)&&getuid()==0);const char *fixture=argv[1],*root=argv[2];
 char cwd[4096];assert(getcwd(cwd,sizeof(cwd)));
 char path[4096];snprintf(path,sizeof(path),"%s/run/nova/app",fixture);assert(chdir(path)==0);
 int listener=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK,0);assert(listener>=0);
 struct sockaddr_un addr={.sun_family=AF_UNIX};snprintf(addr.sun_path,sizeof(addr.sun_path),"client.sock");
 assert(bind(listener,(struct sockaddr*)&addr,sizeof(addr))==0);assert(chmod("client.sock",0666)==0);assert(listen(listener,4)==0);assert(chdir(cwd)==0);
 pid_t kids[3];const char*names[]={"a","b","files"};const char*manifests[]={"demo-app","demo-app-b","nova-files"};
 for(int i=0;i<3;i++){
  clients[i]=-1;kids[i]=fork();assert(kids[i]>=0);
  if(!kids[i]){
   close(listener);assert(unshare(CLONE_NEWNS)==0);assert(mount(0,"/",0,MS_REC|MS_PRIVATE,0)==0);
   assert(mount(fixture,fixture,0,MS_BIND,0)==0);assert(chroot(fixture)==0&&chdir("/")==0);
   char launch[32];snprintf(launch,sizeof(launch),"/launch-%s",names[i]);execl(launch,launch,(char*)0);_exit(127);
  }
  snprintf(path,sizeof(path),"%s/configs/services/%s.service",root,manifests[i]);assert(r17_fixture_register(path,kids[i]));
 }
 int pair[2],control[2];assert(socketpair(AF_UNIX,SOCK_SEQPACKET,0,pair)==0);assert(socketpair(AF_UNIX,SOCK_SEQPACKET,0,control)==0);
 pid_t authority=fork();assert(authority>=0);
 if(!authority){close(listener);close(pair[0]);close(control[0]);r17_fixture_authority(pair[1],control[1]);_exit(0);}
 close(pair[1]);close(control[1]);g_security_fd=pair[0];
 unsigned long long deadline=nova_render_clock(1)+8000000000ULL;
 while(client_connection_count()<3&&nova_render_clock(1)<deadline){
  accept_client_connection(listener,clients);struct timespec pause={0,1000000};nanosleep(&pause,0);
 }
 assert(client_connection_count()==3);until(five_surfaces);
 assert(client_surface_count_for(1)==2&&client_surface_count_for(2)==2&&client_surface_count_for(3)==1);
 for(int i=0;i<3;i++)assert(g_connections[i].authenticated&&g_connections[i].peer_pid>0);
 struct nova_client_surface *files=find_client_surface(300);assert(files&&files->min_width==640&&files->max_width==1600);
 g_scene.configured=1;g_scene.panel_height=58;g_r5_scale=1;g_r5_logical_w=1600;g_r5_logical_h=900;g_r2_canvas_width=1600;g_r2_canvas_height=900;
 g_desktop.connected=1;g_desktop_tasks.count=3;
 for(unsigned int i=0;i<3;i++){
  g_desktop_tasks.entries[i].client_id=i+1;g_desktop_tasks.entries[i].flags=NOVA_DESKTOP_TASK_FLAG_LAUNCHABLE;
  assert(r17_fixture_metadata(i+1,g_desktop_tasks.entries[i].display_name,sizeof(g_desktop_tasks.entries[i].display_name)));
 }
 set_client_focus(300);assert(r17_toggle_maximize(files,1600,900));until(files_maximized);
 assert(r17_minimize(files));assert(!top_client_surface_at(files->x+100,files->y+100)||top_client_surface_at(files->x+100,files->y+100)!=300);
 assert(!route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_R,1));
 assert(desktop_activate_client(3)&&!files->minimized&&g_client.focus_surface==300);
 assert(r17_toggle_maximize(files,1600,900));until(files_restored);
 g_render.cursor_x=files->x+120;g_render.cursor_y=files->y+10;begin_client_grab(files,NOVA_GRAB_MOVE);
 g_render.cursor_x+=480;g_render.cursor_y+=50;update_client_grab(1600,900);finish_client_grab();until(acked);
 assert(files->x==560&&files->y==150);expect_file_view(fixture,0);
 commits_before=g_client_commits_by_id[3];g_render.cursor_x=files->x+284;g_render.cursor_y=files->y+145;
 assert(route_client_event(NOVA_SURFACE_EVENT_POINTER,300,NOVA_BTN_LEFT,1));until(new_files_commit);expect_file_view(fixture,1);
 commits_before=g_client_commits_by_id[3];assert(route_client_event(NOVA_SURFACE_EVENT_KEY,300,NOVA_KEY_BACKSPACE,1));until(new_files_commit);expect_file_view(fixture,0);
 assert(r17_switch_window());set_client_focus(100);
 struct nova_client_surface*a=find_client_surface(100);assert(a);
 g_render.cursor_x=a->x+120;g_render.cursor_y=a->y+10;begin_client_grab(a,NOVA_GRAB_MOVE);
 g_render.cursor_x+=360;g_render.cursor_y+=282;update_client_grab(1600,900);finish_client_grab();until(a_position_acked);
 set_client_focus(300);
 /* Canonical visual comparison order, independent of process arrival. */
 const unsigned int visual_order[]={100,101,200,201,300};
 for(unsigned int vi=0;vi<5;vi++){struct nova_client_surface*vs=find_client_surface(visual_order[vi]);assert(vs);raise_client_surface(vs);}
 unsigned int*pixels=calloc(1600U*900U,sizeof(*pixels));assert(pixels);
 g_render.cursor_x=files->x+(int)files->width-43;g_render.cursor_y=files->y+13;
 draw_scene(pixels,1600,900,6400,0);save_ppm(argv[3],pixels);
 unsigned int*reference=calloc(1600U*900U,sizeof(*reference));assert(reference);
 g_test_reference_blit=1;g_test_reference_shadow=1;g_test_reference_wallpaper=1;draw_scene(reference,1600,900,6400,0);g_test_reference_wallpaper=0;g_test_reference_shadow=0;g_test_reference_blit=0;
 assert(!memcmp(reference,pixels,1600U*900U*4U));free(reference);
 puts("R18_SAME_SCENE_FULL_FRAME_REFERENCE_SHADOW_BYTE_EQUIVALENCE=PASS");
 #include "r21-frame-profile.inc"
 free(pixels);
 commits_before=g_client_commits_by_id[3];g_render.cursor_x=files->x+300;g_render.cursor_y=files->y+145;
 assert(route_client_event(NOVA_SURFACE_EVENT_POINTER,300,NOVA_REL_X,1));until(new_files_commit);
 commits_before=g_client_commits_by_id[3];g_render.cursor_x=files->x-1;
 assert(route_client_event(NOVA_SURFACE_EVENT_POINTER,300,NOVA_REL_X,-1));until(new_files_commit);expect_file_view(fixture,0);
 puts("R18_REAL_FILES_MOTION_HOVER_AND_LEAVE_COMMIT=PASS");
 if(argc==6){
  for(unsigned int i=0;i<NOVA_SURFACE_MAX_CLIENT_SURFACES;i++)
   if(g_surfaces[i].used&&g_surfaces[i].committed&&g_surfaces[i].client_id!=3U)assert(r17_minimize(&g_surfaces[i]));
  set_client_focus(300);g_render.cursor_x=files->x+120;g_render.cursor_y=files->y+10;begin_client_grab(files,NOVA_GRAB_MOVE);
  g_render.cursor_x+=400-files->x;g_render.cursor_y+=180-files->y;update_client_grab(1600,900);finish_client_grab();until(acked);expect_file_view(fixture,0);
  assert(files->x==400&&files->y==180);g_render.cursor_x=1260;g_render.cursor_y=730;
  unsigned int*clean=calloc(1600U*900U,sizeof(*clean));assert(clean);draw_scene(clean,1600,900,6400,0);save_ppm(argv[5],clean);free(clean);
 }
 if(getenv("NOVA_GALLERY_DIR"))r19_capture_gallery(getenv("NOVA_GALLERY_DIR"),fixture);
 if(getenv("NOVA_R25_TERMINAL_TEST"))r25_terminal_multi();
 r22_soak(kids);
 /* Deliberately crash only our own disposable Files child; verify compositor
  * releases its buffers/surfaces and the two other authenticated apps survive. */
 assert(kids[2]>1&&kill(kids[2],SIGKILL)==0);until(no_files);
 int prior_status;assert(waitpid(kids[2],&prior_status,0)==kids[2]&&WIFSIGNALED(prior_status)&&WTERMSIG(prior_status)==SIGKILL);
 assert(!client_buffer_count_for(3)&&client_connection_count()==2&&client_surface_count_for(1)==2&&client_surface_count_for(2)==2);
 puts("R20_OWN_FILES_CHILD_CRASH_BUFFER_SURFACE_CLEANUP_WITH_OTHER_APPS_RUNNING=PASS");
 int gate[2];assert(pipe(gate)==0);kids[2]=fork();assert(kids[2]>=0);
 if(!kids[2]){
  close(gate[1]);char ready;assert(read(gate[0],&ready,1)==1);close(gate[0]);close(listener);
  assert(unshare(CLONE_NEWNS)==0&&mount(0,"/",0,MS_REC|MS_PRIVATE,0)==0);
  assert(mount(fixture,fixture,0,MS_BIND,0)==0&&chroot(fixture)==0&&chdir("/")==0);
  execl("/launch-files","/launch-files",(char*)0);_exit(127);
 }
 close(gate[0]);int update[2]={3,kids[2]},ack[2];assert(write(control[0],update,sizeof(update))==(long)sizeof(update));
 assert(read(control[0],ack,sizeof(ack))==(long)sizeof(ack)&&ack[1]==kids[2]);assert(write(gate[1],"x",1)==1);close(gate[1]);
 deadline=nova_render_clock(1)+8000000000ULL;
 while(client_connection_count()<3&&nova_render_clock(1)<deadline){accept_client_connection(listener,clients);struct timespec pause={0,1000000};nanosleep(&pause,0);}
 assert(client_connection_count()==3);until(five_surfaces);reference_count=0;expect_file_view(fixture,0);
 struct nova_client_connection *reopened=0;
 for(unsigned int i=0;i<NOVA_MAX_CLIENT_CONNECTIONS;i++)if(g_connections[i].client_id==3)reopened=&g_connections[i];
 assert(reopened&&reopened->theme_sent==g_session_theme.generation);
 puts("R21_FILES_REOPEN_CURRENT_SESSION_THEME_REDELIVERED=PASS");
 assert(reopened&&reopened->peer_pid==(unsigned int)kids[2]&&reopened->app_generation==2);
 for(unsigned int cid=1;cid<=3;cid++)assert(desktop_close_client(cid));until(no_clients);
 for(int i=0;i<3;i++){int status;assert(waitpid(kids[i],&status,0)==kids[i]&&WIFEXITED(status)&&WEXITSTATUS(status)==0);}
 close(g_security_fd);close(control[0]);int status;assert(waitpid(authority,&status,0)==authority&&WIFEXITED(status)&&WEXITSTATUS(status)==0);close(listener);
 puts("R17_ACTUAL_DEMO_A_B_FILES_SANDBOX_TO_PRODUCTION_COMPOSITOR_IPC=PASS");
 puts("R17_FILES_COLD_REOPEN_NEW_PID_PRINCIPAL_GENERATION_WITH_DEMOS_RUNNING=PASS");
 puts("R17_REAL_CONFIGURE_RESIZE_MAXIMIZE_RESTORE_MINIMIZE_FOCUS_MOVE_INPUT_CLOSE=PASS");
 puts("R17_HOST_FRAMEBUFFER_RENDER_NOT_VM=PASS; authority uses production novad parser/PID registry/reply builder in host fixture, not PID1 verified boot");return 0;
}
