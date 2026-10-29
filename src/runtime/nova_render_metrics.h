#ifndef NOVA_RENDER_METRICS_H
#define NOVA_RENDER_METRICS_H
#include "nova_runtime.h"
struct nova_render_metrics {
 unsigned long long draws,draw_ns,draw_cpu_ns,draw_max_ns,presents;
 unsigned long long last_present_ns,interval_ns,interval_max_ns;
 unsigned long long fps_window_start_ns,fps_window_count,fps_milli,fps_window_end_ns;
 unsigned long long last_input_ns,input_to_present_ns,input_to_present_max_ns,input_samples;
};
static inline unsigned long long nova_render_clock(unsigned int id){
 struct nova_timespec t;if(nova_clock_gettime(id,&t)<0||t.tv_sec<0||t.tv_nsec<0)return 0;
 return (unsigned long long)t.tv_sec*1000000000ULL+(unsigned long long)t.tv_nsec;
}
static inline int nova_render_needed(unsigned int pending,unsigned long shown,unsigned long scene){
 return !pending&&shown!=scene;
}
static inline void nova_render_sample(struct nova_render_metrics*m,unsigned long long wall,unsigned long long cpu){
 m->draws++;m->draw_ns+=wall;m->draw_cpu_ns+=cpu;if(wall>m->draw_max_ns)m->draw_max_ns=wall;
}
static inline void nova_present_sample(struct nova_render_metrics*m,unsigned long long now,unsigned long long input){
 m->presents++;
 if(!m->fps_window_count){m->fps_window_start_ns=now;m->fps_window_count=1;}
 else{
  m->fps_window_count++;
  unsigned long long dt=now>=m->fps_window_start_ns?now-m->fps_window_start_ns:0;
  if(dt>=1000000000ULL){
   m->fps_milli=(m->fps_window_count-1U)*1000000000000ULL/dt;
   m->fps_window_end_ns=now;m->fps_window_start_ns=now;m->fps_window_count=1;
  }
 }
 if(m->last_present_ns&&now>=m->last_present_ns){
  unsigned long long dt=now-m->last_present_ns;m->interval_ns+=dt;
  if(dt>m->interval_max_ns)m->interval_max_ns=dt;
 }
 m->last_present_ns=now;
 if(input&&input<=now){
  unsigned long long dt=now-input;m->input_samples++;m->input_to_present_ns+=dt;
  if(dt>m->input_to_present_max_ns)m->input_to_present_max_ns=dt;
 }
}
#endif
