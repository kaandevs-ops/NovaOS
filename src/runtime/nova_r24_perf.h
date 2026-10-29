#ifndef NOVA_R24_PERF_H
#define NOVA_R24_PERF_H
#include "nova_render_metrics.h"
#ifndef NOVA_R24_OPTIMIZED
#define NOVA_R24_OPTIMIZED 1
#endif
#define R24_SAMPLES 256U
#define R24_REPORT_BYTES 2048U
struct r24_hist {unsigned long long v[R24_SAMPLES];unsigned int n,pos;};
struct r24_perf {struct r24_hist draw,latency,age;unsigned long long last_report,start,events,batches,budget_hits,drain_ns,drain_max,log_ns,oldest,full,partial,queue_dropped,invalid_clock,last_snapshot;int input_fd[32];unsigned int input_count,sync_count,sync_lost[32];int sync_fd[32];unsigned long long resync_failed,state_calls,state_ns,state_max,age_max,latency_max,flip_started,flip_max,event_backpressure,event_errors,authority_warm_ns,auth_calls,auth_ns,auth_max,wallpaper_frames,wallpaper_palette_builds,wallpaper_map_builds,wallpaper_fallbacks,wallpaper_cache_bytes;};
static struct r24_perf g_r24;
static void r24_hist_add(struct r24_hist*h,unsigned long long v){h->v[h->pos++%R24_SAMPLES]=v;if(h->n<R24_SAMPLES)h->n++;}
static unsigned long long r24_percentile(const struct r24_hist*h,unsigned int percentile){unsigned long long a[R24_SAMPLES];for(unsigned int i=0;i<h->n;i++){a[i]=h->v[i];unsigned int j=i;while(j&&a[j]<a[j-1]){unsigned long long t=a[j];a[j]=a[j-1];a[j-1]=t;j--;}}return h->n?a[(h->n*percentile+99)/100-1]:0;}
static void r24_input_source(int fd,long sec,long usec){g_r24.events++;int valid=0;for(unsigned int i=0;i<g_r24.input_count;i++)if(g_r24.input_fd[i]==fd)valid=1;if(!valid||sec<0||usec<0||usec>=1000000)return;unsigned long long source=(unsigned long long)sec*1000000000ULL+(unsigned long long)usec*1000ULL,now=nova_render_clock(1);if(!source||source>now){g_r24.invalid_clock++;return;}r24_hist_add(&g_r24.age,now-source);if(now-source>g_r24.age_max)g_r24.age_max=now-source;if(!g_r24.oldest||source<g_r24.oldest)g_r24.oldest=source;}
static void r24_text(char*b,unsigned int*n,const char*s){while(*s&&*n<R24_REPORT_BYTES-1U)b[(*n)++]=*s++;}
static void r24_number(char*b,unsigned int*n,unsigned long long v){char a[24];unsigned int k=0;do{a[k++]=(char)('0'+v%10);v/=10;}while(v);while(k&&*n<R24_REPORT_BYTES-1U)b[(*n)++]=a[--k];}
static void r24_report(void){unsigned long long now=nova_render_clock(1);if(!now||now-g_r24.last_report<1000000000ULL)return;g_r24.last_report=now;char b[R24_REPORT_BYTES];unsigned int n=0;r24_text(b,&n,"R24_PERF {");
#define R24_FIELD(label,value) r24_text(b,&n,"\"" label "\":");r24_number(b,&n,(value));r24_text(b,&n,",")
R24_FIELD("optimized",NOVA_R24_OPTIMIZED);R24_FIELD("mono_ns",now);R24_FIELD("events",g_r24.events);R24_FIELD("batches",g_r24.batches);R24_FIELD("budget_hits",g_r24.budget_hits);R24_FIELD("drain_ns",g_r24.drain_ns);R24_FIELD("drain_max_ns",g_r24.drain_max);R24_FIELD("legacy_log_ns",g_r24.log_ns);R24_FIELD("draw_count",g_metrics.draws);R24_FIELD("draw_samples",g_r24.draw.n);R24_FIELD("draw_p50_ns",r24_percentile(&g_r24.draw,50));R24_FIELD("draw_p95_ns",r24_percentile(&g_r24.draw,95));R24_FIELD("present_count",g_metrics.presents);R24_FIELD("event_completion_samples",g_r24.latency.n);R24_FIELD("event_completion_p50_ns",r24_percentile(&g_r24.latency,50));R24_FIELD("event_completion_p95_ns",r24_percentile(&g_r24.latency,95));R24_FIELD("queue_age_p95_ns",r24_percentile(&g_r24.age,95));R24_FIELD("full_frames",g_r24.full);R24_FIELD("cursor_frames",g_r24.partial);R24_FIELD("syn_dropped",g_r24.queue_dropped);R24_FIELD("clock_devices",g_r24.input_count);R24_FIELD("draw_cpu_ns",g_metrics.draw_cpu_ns);R24_FIELD("resync_failed",g_r24.resync_failed);R24_FIELD("state_calls",g_r24.state_calls);R24_FIELD("state_ns",g_r24.state_ns);R24_FIELD("state_max_ns",g_r24.state_max);R24_FIELD("state_write_syscalls",g_nova_state.writes);R24_FIELD("queue_age_max_ns",g_r24.age_max);R24_FIELD("event_completion_max_ns",g_r24.latency_max);R24_FIELD("draw_max_ns",g_metrics.draw_max_ns);R24_FIELD("flip_wait_max_ns",g_r24.flip_max);R24_FIELD("event_backpressure",g_r24.event_backpressure);R24_FIELD("event_errors",g_r24.event_errors);R24_FIELD("authority_warm_ns",g_r24.authority_warm_ns);R24_FIELD("auth_calls",g_r24.auth_calls);R24_FIELD("auth_ns",g_r24.auth_ns);R24_FIELD("auth_max_ns",g_r24.auth_max);R24_FIELD("wallpaper_frames",g_r24.wallpaper_frames);R24_FIELD("wallpaper_palette_builds",g_r24.wallpaper_palette_builds);R24_FIELD("wallpaper_map_builds",g_r24.wallpaper_map_builds);R24_FIELD("wallpaper_fallbacks",g_r24.wallpaper_fallbacks);R24_FIELD("wallpaper_cache_bytes",g_r24.wallpaper_cache_bytes);
#undef R24_FIELD
if(n&&b[n-1]==',')n--;r24_text(b,&n,"}\n");if(!NOVA_R24_OPTIMIZED){(void)nova_write(1,b,n);}else{long fd=nova_open_write("/run/nova/performance.state.tmp",0644);if(fd>=0){long written=nova_write((int)fd,b,n);nova_close((int)fd);if(written==(long)n){
#if defined(__aarch64__)
(void)nova_syscall5(276,-100,(long)"/run/nova/performance.state.tmp",-100,(long)"/run/nova/performance.state",0);
#else
(void)nova_syscall5(316,-100,(long)"/run/nova/performance.state.tmp",-100,(long)"/run/nova/performance.state",0);
#endif
}}}}

#endif
