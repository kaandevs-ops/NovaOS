/* MuPDF-linked derivative, GNU AGPL v3 or later. See third_party/mupdf. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
#include "nova_pdf_protocol.h"
#include "nova_pdf_worker_policy.h"
#define HEAP_CAP (8U*1024U*1024U)
union allocation {max_align_t align;size_t size;};
static size_t used;
static void*allocate(void*u,size_t n){(void)u;if(n>HEAP_CAP-used)return NULL;union allocation*h=malloc(sizeof(*h)+n);if(!h)return NULL;h->size=n;used+=n;return h+1;}
static void release(void*u,void*p){(void)u;if(p){union allocation*h=(union allocation*)p-1;used-=h->size;free(h);}}
static void*resize(void*u,void*p,size_t n){if(!p)return allocate(u,n);if(!n){release(u,p);return NULL;}union allocation*h=(union allocation*)p-1;size_t old=h->size;if(n>HEAP_CAP-(used-old))return NULL;h=realloc(h,sizeof(*h)+n);if(!h)return NULL;h->size=n;used=used-old+n;return h+1;}
static void silent(void*u,const char*s){(void)u;(void)s;}
static int number(const char*s,unsigned max,unsigned*out){if(!*s)return 0;unsigned v=0;for(;*s;s++){if(*s<'0'||*s>'9'||v>max/10)return 0;v=v*10+(unsigned)(*s-'0');if(v>max)return 0;}*out=v;return 1;}
static int write_all(const void*p,size_t n){const unsigned char*s=p;while(n){ssize_t k=write(1,s,n);if(k<0&&errno==EINTR)continue;if(k<=0)return -1;s+=k;n-=(size_t)k;}return 0;}
int main(int argc,char**argv){
 unsigned page,w,h,zoom,panx,pany;if(argc!=7||!number(argv[1],511,&page)||!number(argv[2],800,&w)||!w||!number(argv[3],600,&h)||!h||!number(argv[4],4000,&zoom)||zoom<1000||!number(argv[5],3200,&panx)||!number(argv[6],2400,&pany))return 2;
 struct rlimit limit={3,3};if(setrlimit(RLIMIT_CPU,&limit))return 3;
 struct stat st;int seals=fcntl(3,F_GET_SEALS);if(fstat(3,&st)||!S_ISREG(st.st_mode)||(st.st_mode&0777)!=0400||st.st_size<8||st.st_size>NOVA_PDF_MAX_BYTES||seals<0||(seals&0x2f)!=0x2f)return 4;
 unsigned char*data=mmap(NULL,(size_t)st.st_size,PROT_READ,MAP_PRIVATE,3,0);if(data==MAP_FAILED)return 5;close(3);if(memcmp(data,"%PDF-",5))return 6;
 fz_alloc_context allocator={NULL,allocate,resize,release};fz_context*ctx=fz_new_context(&allocator,NULL,2U*1024U*1024U);if(!ctx)return 7;
 fz_set_error_callback(ctx,silent,NULL);fz_set_warning_callback(ctx,silent,NULL);
 if(nova_pdf_worker_policy())return 8;
 pdf_document*doc=NULL;pdf_page*pg=NULL;fz_stream*stream=NULL;fz_pixmap*pix=NULL;fz_device*dev=NULL;int result=9;
 fz_var(doc);fz_var(pg);fz_var(stream);fz_var(pix);fz_var(dev);fz_var(result);
 fz_try(ctx){
  stream=fz_open_memory(ctx,data,(size_t)st.st_size);doc=pdf_open_document_with_stream(ctx,stream);
  if(pdf_needs_password(ctx,doc))fz_throw(ctx,FZ_ERROR_ARGUMENT,"password required");int count=pdf_count_pages(ctx,doc);if(count<1||count>512||page>=(unsigned)count)fz_throw(ctx,FZ_ERROR_LIMIT,"page limit");
  pg=pdf_load_page(ctx,doc,(int)page);fz_rect box=pdf_bound_page(ctx,pg,FZ_CROP_BOX);float bw=box.x1-box.x0,bh=box.y1-box.y0;
  if(!isfinite(bw)||!isfinite(bh)||bw<=0||bh<=0||bw>100000||bh>100000)fz_throw(ctx,FZ_ERROR_LIMIT,"geometry limit");
  float scale=fminf((float)w/bw,(float)h/bh)*(float)zoom/1000.0f;unsigned fw=(unsigned)ceilf(bw*scale),fh=(unsigned)ceilf(bh*scale);if(!fw||!fh||fw>3200||fh>2400)fz_throw(ctx,FZ_ERROR_LIMIT,"raster limit");unsigned ow=fw<w?fw:w,oh=fh<h?fh:h;panx=panx>fw-ow?fw-ow:panx;pany=pany>fh-oh?fh-oh:pany;
  fz_irect bbox={0,0,(int)ow,(int)oh};pix=fz_new_pixmap_with_bbox(ctx,fz_device_rgb(ctx),bbox,NULL,0);fz_clear_pixmap_with_value(ctx,pix,255);
  fz_matrix matrix=fz_concat(fz_translate(-box.x0,-box.y0),fz_scale(scale,scale));matrix=fz_concat(matrix,fz_translate(-(float)panx,-(float)pany));dev=fz_new_draw_device(ctx,fz_identity,pix);fz_cookie cookie={0};pdf_run_page(ctx,pg,dev,matrix,&cookie);fz_close_device(ctx,dev);
  struct nova_pdf_result out={NOVA_PDF_MAGIC,1,0,page,(unsigned)count,ow,oh,ow*3,ow*oh*3,fw,fh,panx,pany};
  if(fz_pixmap_components(ctx,pix)!=3||fz_pixmap_stride(ctx,pix)<(int)out.stride)fz_throw(ctx,FZ_ERROR_FORMAT,"unexpected pixels");
  if(write_all(&out,sizeof out))fz_throw(ctx,FZ_ERROR_SYSTEM,"pipe closed");unsigned char*samples=fz_pixmap_samples(ctx,pix);int stride=fz_pixmap_stride(ctx,pix);for(unsigned y=0;y<oh;y++)if(write_all(samples+(size_t)y*stride,out.stride))fz_throw(ctx,FZ_ERROR_SYSTEM,"pipe closed");result=0;
 }fz_always(ctx){fz_drop_device(ctx,dev);fz_drop_pixmap(ctx,pix);fz_drop_page(ctx,(fz_page*)pg);pdf_drop_document(ctx,doc);fz_drop_stream(ctx,stream);}fz_catch(ctx){result=9;}
 fz_drop_context(ctx);munmap(data,(size_t)st.st_size);return result;
}
