#ifndef NOVA_PDF_PROTOCOL_H
#define NOVA_PDF_PROTOCOL_H
#include <stdint.h>
#define NOVA_PDF_MAGIC 0x4e504446U
#define NOVA_PDF_MAX_BYTES (4U*1024U*1024U)
#define NOVA_PDF_MAX_W 800U
#define NOVA_PDF_MAX_H 600U
#define NOVA_PDF_MAX_PAGES 512U
struct nova_pdf_result {uint32_t magic,version,status,page,pages,width,height,stride,bytes,full_width,full_height,pan_x,pan_y;};
_Static_assert(sizeof(struct nova_pdf_result)==52,"private PDF pipe ABI");
static inline int nova_pdf_result_valid(const struct nova_pdf_result*h,unsigned page,unsigned w,unsigned height){
 return h->magic==NOVA_PDF_MAGIC&&h->version==1&&!h->status&&h->page==page&&h->pages&&h->pages<=NOVA_PDF_MAX_PAGES&&page<h->pages&&h->width&&h->width<=w&&h->width<=NOVA_PDF_MAX_W&&h->height&&h->height<=height&&h->height<=NOVA_PDF_MAX_H&&h->stride==h->width*3&&h->bytes==h->stride*h->height&&h->full_width>=h->width&&h->full_width<=NOVA_PDF_MAX_W*4&&h->full_height>=h->height&&h->full_height<=NOVA_PDF_MAX_H*4&&h->pan_x<=h->full_width-h->width&&h->pan_y<=h->full_height-h->height;
}
#endif
