#include <stdlib.h>
#include <unistd.h>
#include "../src/pdf/nova_pdf_protocol.h"
int main(int argc,char**argv){if(argc<2)return 2;int mode=atoi(argv[1]);struct nova_pdf_result h={NOVA_PDF_MAGIC,1,0,(unsigned)mode,4,1,1,3,3,1,1,0,0};
 if(mode==0){h.width=0xffffffffU;return write(1,&h,sizeof h)!=sizeof h;}
 if(mode==1){return write(1,&h,sizeof h)!=sizeof h||write(1,"123extra",8)!=8;}
 if(mode==2){return write(1,&h,sizeof h)!=sizeof h||write(1,"12",2)!=2;}
 close(1);sleep(20);return 0;}
