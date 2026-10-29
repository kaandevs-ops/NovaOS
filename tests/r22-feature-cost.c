#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
#include "../files/src/nova_files_guest_metadata.h"
static int cmp(const void*a,const void*b){double x=*(const double*)a,y=*(const double*)b;return(x>y)-(x<y);}
int main(void){unsigned int w=1600,h=900;unsigned char*b=calloc(h,w*4);assert(b);g_scene.panel_height=38;double values[3][101];char path[]="/tmp/nova-r22-cost-XXXXXX";assert(mkdtemp(path));int fd=open(path,O_RDONLY|O_DIRECTORY);assert(fd>=0);for(unsigned int i=0;i<48;i++){char name[32];snprintf(name,sizeof(name),"File-%u",i);int f=openat(fd,name,O_WRONLY|O_CREAT|O_EXCL,0600);assert(f>=0&&!ftruncate(f,1536));close(f);}
 for(unsigned int part=0;part<3;part++)for(unsigned int i=0;i<111;i++){unsigned long long start=nova_render_clock(1);if(part<2){g_r7_controls_visible=part==0?2:1;r7_controls(b,w,h,w*4);}else for(unsigned int j=0;j<48;j++){char name[32];snprintf(name,sizeof(name),"File-%u",j);struct nova_files_metadata m;assert(nova_files_guest_metadata(fd,name,8,&m)&&m.bytes==1536);}double elapsed=(nova_render_clock(1)-start)/1000000.0;if(i>=10)values[part][i-10]=elapsed;}
 for(unsigned int p=0;p<3;p++)qsort(values[p],101,sizeof(double),cmp);printf("{\"scope\":\"host isolated component timings, 10 warmup+101 samples; NOT FPS or end-to-end latency\",\"quick_panel_p50_ms\":%.6f,\"quick_panel_p95_ms\":%.6f,\"full_settings_p50_ms\":%.6f,\"full_settings_p95_ms\":%.6f,\"48_metadata_p50_ms\":%.6f,\"48_metadata_p95_ms\":%.6f}\n",values[0][50],values[0][95],values[1][50],values[1][95],values[2][50],values[2][95]);close(fd);free(b);return 0;}
