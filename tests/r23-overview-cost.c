#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#define nova_main nova_init_unused
#include "../src/init/main.c"
#undef nova_main
static unsigned int fds(void){DIR*d=opendir("/proc/self/fd");assert(d);unsigned int n=0;while(readdir(d))n++;closedir(d);return n;}
int main(void){assert(getuid()==0);g_service_count=3;nova_zero(g_services,sizeof(g_services));copy_string(g_services[0].name,sizeof(g_services[0].name),"compositor");g_services[0].pid=getpid();g_services[1].pid=123;g_services[1].is_application=1;g_services[2].restart_count=2;
 int pair[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK,0,pair));assert(overview_peer_authorized(pair[0]));struct nova_overview_request q={NOVA_OVERVIEW_MAGIC,1,sizeof(q),42,0,0};struct nova_overview_reply reply;
 struct timespec start,end;assert(!clock_gettime(CLOCK_MONOTONIC,&start));for(unsigned int i=0;i<1000;i++){q.sequence=i+1;assert(send(pair[1],&q,sizeof(q),0)==sizeof(q));assert(!process_overview_packet(pair[0]));assert(recv(pair[1],&reply,sizeof(reply),0)==sizeof(reply)&&nova_overview_reply_valid(&reply,q.sequence));assert(reply.services_total==3&&reply.services_running==2&&reply.apps_running==1&&reply.restarts==2&&reply.secure_boot==0);}
 assert(!clock_gettime(CLOCK_MONOTONIC,&end));double elapsed=(end.tv_sec-start.tv_sec)*1000.0+(end.tv_nsec-start.tv_nsec)/1000000.0;printf("{\"scope\":\"optimized host socketpair actual novad handler, 1000 serial requests; NOT PID1 event loop or guest latency\",\"requests\":1000,\"total_ms\":%.6f,\"mean_ms\":%.6f}\n",elapsed,elapsed/1000.0);
 g_services[1].pid=0;g_services[1].restart_count++;fill_overview_reply(&reply,2);assert(reply.services_running==1&&reply.apps_running==0&&reply.restarts==3);g_services[1].pid=456;fill_overview_reply(&reply,3);assert(reply.apps_running==1);
 g_firmware_trust=FIRMWARE_TRUST_UNAVAILABLE;g_firmware_secure_boot=1;fill_overview_reply(&reply,4);assert(reply.secure_boot==3);g_firmware_trust=FIRMWARE_TRUST_VERIFIED;g_tpm_state=TPM_STATE_READY;g_verity=VERITY_ENFORCED;fill_overview_reply(&reply,4);assert(reply.secure_boot==1&&reply.tpm==1&&reply.verity==1);
 g_services[0].pid++;assert(!overview_peer_authorized(pair[0])&&process_overview_packet(pair[0])==-1);g_services[0].pid=getpid();q.reserved0=1;assert(send(pair[1],&q,sizeof(q),0)==sizeof(q)&&process_overview_packet(pair[0])==-1);q.reserved0=0;assert(send(pair[1],&q,sizeof(q)-1,0)==sizeof(q)-1&&process_overview_packet(pair[0])==-1);
 int file=open("/dev/null",O_RDONLY);assert(file>=0);unsigned int before=fds();char control[CMSG_SPACE(sizeof(int))]={0};struct iovec io={&q,sizeof(q)};struct msghdr m={0};m.msg_iov=&io;m.msg_iovlen=1;m.msg_control=control;m.msg_controllen=sizeof(control);struct cmsghdr*h=CMSG_FIRSTHDR(&m);h->cmsg_level=SOL_SOCKET;h->cmsg_type=SCM_RIGHTS;h->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(h),&file,sizeof(file));assert(sendmsg(pair[1],&m,0)==sizeof(q)&&process_overview_packet(pair[0])==-1&&fds()==before);close(file);close(pair[1]);assert(process_overview_packet(pair[0])==-1);close(pair[0]);
 puts("R23_NOVAD_READONLY_OVERVIEW_PEER_AUTH_REGISTRY_RESTART_BOOT_MAPPING_1000_REQUESTS_MALFORMED_ANCILLARY_FD=PASS");return 0;}
