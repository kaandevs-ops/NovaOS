#define _GNU_SOURCE
#define main pdf_frontend_unused_main
#include "../src/pdf/nova_pdf.c"
#undef main
#include <assert.h>
int main(void){unsigned char*rgb=malloc(800*600*3);assert(rgb);struct nova_pdf_result h;
 int fd=open("/dev/null",O_RDONLY);assert(fd>=0);
 for(unsigned page=0;page<4;page++){long long start=millis();assert(render(fd,page,800,600,1000,0,0,&h,rgb)<0);assert(millis()-start<6500);}
 close(fd);free(rgb);puts("R44_SUPERVISOR_PASS corrupt/trailing/truncated/timeout; own children reaped");return 0;}
