#include <stdio.h>
#include <string.h>
#include "nova_sha256.h"
#include "nova_tcg2.h"

static unsigned char logbuf[4096];
static unsigned long loglen;
static void le16put(unsigned char *p,unsigned int v){p[0]=v&255;p[1]=(v>>8)&255;}
static void le32put(unsigned char *p,unsigned long v){p[0]=v&255;p[1]=(v>>8)&255;p[2]=(v>>16)&255;p[3]=(v>>24)&255;}
static void add_event2(unsigned long pcr,unsigned char fill){
    unsigned long o=loglen; le32put(logbuf+o,pcr);le32put(logbuf+o+4,1);le32put(logbuf+o+8,1);o+=12;
    le16put(logbuf+o,0x000b);o+=2;memset(logbuf+o,fill,32);o+=32;le32put(logbuf+o,0);o+=4;loglen=o;
}
static int forensic_fields_match(const struct nova_tcg2_evidence *ev,const unsigned char *data,unsigned long expected_len){
    unsigned char digest[32]; char expected[65]; struct nova_sha256_ctx c;
    nova_sha256_init(&c);nova_sha256_update(&c,data,expected_len);nova_sha256_final(&c,digest);nova_sha256_hex(digest,expected);
    return ev->log_bytes==expected_len && strlen(ev->log_sha256)==64U && strcmp(ev->log_sha256,expected)==0;
}
static void build_log(void){
    memset(logbuf,0,sizeof(logbuf)); unsigned long specsz=37; le32put(logbuf,0);le32put(logbuf+4,3);le32put(logbuf+28,specsz);
    unsigned long o=32;memcpy(logbuf+o,"Spec ID Event03",15);logbuf[o+15]=0;o+=16;
    le32put(logbuf+o,0);o+=4;logbuf[o++]=0;logbuf[o++]=2;logbuf[o++]=0;logbuf[o++]=2;
    le32put(logbuf+o,2);o+=4;le16put(logbuf+o,0x0004);le16put(logbuf+o+2,20);o+=4;le16put(logbuf+o,0x000b);le16put(logbuf+o+2,32);o+=4;logbuf[o++]=0;
    loglen=o;add_event2(7,0x11);add_event2(7,0x22);add_event2(14,0x33);
}
int main(void){
    build_log();struct nova_tcg2_evidence ev;if(!nova_tcg2_parse_event_log(logbuf,loglen,&ev)){fprintf(stderr,"parse failed: %s\n",nova_tcg2_last_error());return 1;}
    unsigned char pcr[32]={0},d[32],next[32];struct nova_sha256_ctx c;
    memset(d,0x11,32);nova_sha256_init(&c);nova_sha256_update(&c,pcr,32);nova_sha256_update(&c,d,32);nova_sha256_final(&c,next);memcpy(pcr,next,32);
    memset(d,0x22,32);nova_sha256_init(&c);nova_sha256_update(&c,pcr,32);nova_sha256_update(&c,d,32);nova_sha256_final(&c,next);char expected[65];nova_sha256_hex(next,expected);
    if(strcmp(expected,ev.replayed_pcr7_sha256)!=0||ev.pcr7_sha256_events!=2||ev.event2_count!=3){fprintf(stderr,"replay mismatch\n");return 2;}
    unsigned long good_len=loglen; unsigned char saved=logbuf[32];
    logbuf[32]='X';if(nova_tcg2_parse_event_log(logbuf,loglen,&ev)){fprintf(stderr,"corrupt SpecID accepted\n");return 3;}
    if(!forensic_fields_match(&ev,logbuf,loglen)){fprintf(stderr,"corrupt SpecID lost raw forensic digest/length\n");return 6;}
    logbuf[32]=saved;
    const unsigned long cuts[]={1,31,32,40,68,good_len-1};
    for(unsigned int i=0;i<sizeof(cuts)/sizeof(cuts[0]);i++){
        if(cuts[i]>=good_len) continue;
        if(nova_tcg2_parse_event_log(logbuf,cuts[i],&ev)){fprintf(stderr,"truncated event log accepted at %lu\n",cuts[i]);return 4;}
        if(!forensic_fields_match(&ev,logbuf,cuts[i])){fprintf(stderr,"truncated event log lost raw forensic digest/length at %lu\n",cuts[i]);return 7;}
    }
    build_log(); le32put(logbuf+32+37+8,17);
    if(nova_tcg2_parse_event_log(logbuf,loglen,&ev)){fprintf(stderr,"malformed digest count accepted\n");return 5;}
    if(!forensic_fields_match(&ev,logbuf,loglen)){fprintf(stderr,"malformed event log lost raw forensic digest/length\n");return 8;}
    puts("Stage-4D TCG2 event-log golden vector + malformed/truncated rejection + rejected-log forensic digest contract passed.");return 0;
}
