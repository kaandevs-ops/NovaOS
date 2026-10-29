#ifndef NOVA_PREFERENCES_STORE_H
#define NOVA_PREFERENCES_STORE_H
#include "nova_runtime.h"
/* Future broker component, NOT connected to the guest/session UI in R23.
 * Caller supplies a trusted, private directory FD and expected filesystem owner.
 * Root-owned per-user broker directories require separate authenticated UID routing. */
struct nova_preferences {unsigned int theme,generation;};
static unsigned int nova_pref_u32(const unsigned char*b){return b[0]|((unsigned int)b[1]<<8)|((unsigned int)b[2]<<16)|((unsigned int)b[3]<<24);}
static void nova_pref_put(unsigned char*b,unsigned int n){for(unsigned int i=0;i<4;i++)b[i]=(unsigned char)(n>>(i*8));}
static unsigned int nova_pref_hash(const unsigned char*b){unsigned int h=2166136261U;for(unsigned int i=0;i<20;i++){h^=b[i];h*=16777619U;}return h;} /* corruption check, not a signature */
static int nova_preferences_encode(const struct nova_preferences*p,unsigned char*b){if(!p||!b||p->theme>2||!p->generation)return 0;nova_pref_put(b,0x4e505246U);nova_pref_put(b+4,1);nova_pref_put(b+8,p->theme);nova_pref_put(b+12,p->generation);nova_pref_put(b+16,0);nova_pref_put(b+20,nova_pref_hash(b));return 1;}
static int nova_preferences_decode(struct nova_preferences*p,const unsigned char*b,unsigned int n){if(!p||!b||n!=24||nova_pref_u32(b)!=0x4e505246U||nova_pref_u32(b+4)!=1||nova_pref_u32(b+8)>2||!nova_pref_u32(b+12)||nova_pref_u32(b+16)||nova_pref_u32(b+20)!=nova_pref_hash(b))return 0;p->theme=nova_pref_u32(b+8);p->generation=nova_pref_u32(b+12);return 1;}
#if defined(__x86_64__)
#define NOVA_PREF_FSYNC 74L
#define NOVA_PREF_UNLINKAT 263L
#define NOVA_PREF_RENAMEAT2 316L
#else
#define NOVA_PREF_FSYNC 82L
#define NOVA_PREF_UNLINKAT 35L
#define NOVA_PREF_RENAMEAT2 276L
#endif
static int nova_pref_check_fd(int fd,unsigned int owner,int directory){struct nova_statx st;nova_zero(&st,sizeof(st));if(nova_syscall5(NOVA_NR_statx,fd,(long)"",NOVA_AT_EMPTY_PATH,0x7ff,(long)&st)<0)return 0;return (st.stx_mask&0xf)==0xf&&st.stx_uid==owner&&(st.stx_mode&0170000U)==(directory?0040000U:0100000U)&&(st.stx_mode&0777U)==(directory?0700U:0600U)&&(directory||st.stx_nlink==1);}
static long nova_pref_open(int dir,const char*name,unsigned long flags,unsigned long mode){struct {unsigned long long flags,mode,resolve;}how={flags,mode,0x0e};return nova_syscall4(437,dir,(long)name,(long)&how,sizeof(how));}
/* 0 valid, 1 absent, -1 malformed/untrusted/I/O. No guessed defaults here. */
static int nova_preferences_load(int dir,unsigned int owner,struct nova_preferences*out){if(!out||!nova_pref_check_fd(dir,owner,1))return -1;long fd=nova_pref_open(dir,"appearance.v1",NOVA_O_RDONLY|NOVA_O_CLOEXEC|NOVA_O_NOFOLLOW,0);if(fd==-2)return 1;if(fd<0)return -1;unsigned char b[25];int valid=nova_pref_check_fd((int)fd,owner,0)&&nova_read((int)fd,b,sizeof(b))==24&&nova_preferences_decode(out,b,24);nova_close((int)fd);return valid?0:-1;}
#ifdef NOVA_PREFS_TEST_FAULTS
static unsigned int g_preferences_fail_stage;
#endif
static long nova_pref_sync(int fd,unsigned int stage){
#ifdef NOVA_PREFS_TEST_FAULTS
 if(g_preferences_fail_stage&&g_preferences_fail_stage==stage)return -5;
#else
 (void)stage;
#endif
 return nova_syscall1(NOVA_PREF_FSYNC,fd);
}
/* 0 durable success, -1 no rename committed, -2 rename visible but directory fsync failed. */
static int nova_preferences_store(int dir,unsigned int owner,const struct nova_preferences*p){unsigned char b[24];if(!nova_preferences_encode(p,b)||!nova_pref_check_fd(dir,owner,1)||nova_pref_sync(dir,0)<0)return -1;struct nova_preferences prior;int state=nova_preferences_load(dir,owner,&prior);if(state<0||(!state&&p->generation<=prior.generation))return -1;
 long fd=nova_pref_open(dir,".appearance.new",NOVA_O_WRONLY|NOVA_O_CREAT|NOVA_O_EXCL|NOVA_O_CLOEXEC|NOVA_O_NOFOLLOW,0600);if(fd<0)return -1;
 int current=nova_preferences_load(dir,owner,&prior);
 int valid=current>=0&&(current||p->generation>prior.generation)&&nova_pref_check_fd((int)fd,owner,0)&&nova_write((int)fd,b,sizeof(b))==sizeof(b)&&nova_pref_sync((int)fd,1)==0;nova_close((int)fd);
 if(!valid||nova_syscall5(NOVA_PREF_RENAMEAT2,dir,(long)".appearance.new",dir,(long)"appearance.v1",0)<0){(void)nova_syscall3(NOVA_PREF_UNLINKAT,dir,(long)".appearance.new",0);return -1;}
 return nova_pref_sync(dir,2)<0?-2:0;
}
#endif
