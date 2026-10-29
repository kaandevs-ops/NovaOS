#include "nova_x11_pixels.h"
#include "nova_surface_protocol.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
extern Bool XTestFakeKeyEvent(Display*,unsigned int,Bool,unsigned long);
extern Bool XTestFakeButtonEvent(Display*,unsigned int,Bool,unsigned long);
extern Bool XTestFakeMotionEvent(Display*,int,int,int,unsigned long);
struct nova_x11 {Display*d;int screen;unsigned w,h;unsigned char keys[248],buttons[6];};
struct nova_x11*nova_x11_open(const char*display){if(!display)return NULL;Display*d=XOpenDisplay(display);if(!d)return NULL;struct nova_x11*c=calloc(1,sizeof*c);if(!c){XCloseDisplay(d);return NULL;}c->d=d;c->screen=DefaultScreen(d);c->w=(unsigned)DisplayWidth(d,c->screen);c->h=(unsigned)DisplayHeight(d,c->screen);if(!c->w||!c->h||c->w>1920||c->h>1080||(uint64_t)c->w*c->h*4>NOVA_SURFACE_MAX_BUFFER_BYTES){nova_x11_close(c);return NULL;}return c;}
void nova_x11_release_inputs(struct nova_x11*c){if(!c)return;for(unsigned i=1;i<248;i++)if(c->keys[i]){XTestFakeKeyEvent(c->d,i+8,False,CurrentTime);c->keys[i]=0;}for(unsigned i=1;i<6;i++)if(c->buttons[i]){XTestFakeButtonEvent(c->d,i,False,CurrentTime);c->buttons[i]=0;}XFlush(c->d);}
void nova_x11_close(struct nova_x11*c){if(c){nova_x11_release_inputs(c);XCloseDisplay(c->d);free(c);}}
int nova_x11_size(struct nova_x11*c,unsigned*w,unsigned*h){if(!c||!w||!h)return -1;*w=c->w;*h=c->h;return 0;}
static unsigned channel(unsigned long pixel,unsigned long mask){if(!mask)return 0;while(!(mask&1)){mask>>=1;pixel>>=1;}return (unsigned)((pixel&mask)*255/mask);}
/* Canonical little-endian XRGB needs no per-pixel virtual XGetPixel call or
 * channel divisions. Retain the reference path for other masks/byte orders. */
static void copy_image(XImage*i,unsigned w,unsigned h,void*out,int reference){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
 if(!reference&&i->byte_order==LSBFirst&&i->red_mask==0xff0000UL&&i->green_mask==0xff00UL&&i->blue_mask==0xffUL){
  for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){uint32_t rgb;memcpy(&rgb,i->data+(size_t)y*i->bytes_per_line+x*4,4);rgb&=0x00ffffffU;memcpy((unsigned char*)out+((size_t)y*w+x)*4,&rgb,4);}return;
 }
#else
 (void)reference;
#endif
 for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){unsigned long pixel=XGetPixel(i,(int)x,(int)y);uint32_t rgb=channel(pixel,i->red_mask)<<16|channel(pixel,i->green_mask)<<8|channel(pixel,i->blue_mask);memcpy((unsigned char*)out+((size_t)y*w+x)*4,&rgb,4);}
}
int nova_x11_pixels(struct nova_x11*c,void*out,size_t capacity){if(!c||!out||(uint64_t)c->w*c->h*4>capacity)return -1;XImage*i=XGetImage(c->d,RootWindow(c->d,c->screen),0,0,c->w,c->h,AllPlanes,ZPixmap);if(!i)return -1;
 if(i->width!=(int)c->w||i->height!=(int)c->h||i->depth!=24||i->bits_per_pixel!=32||i->bytes_per_line<(int)c->w*4){XDestroyImage(i);return -1;}
 copy_image(i,c->w,c->h,out,0);XDestroyImage(i);return 0;}
int nova_x11_key(struct nova_x11*c,unsigned evdev,int pressed){if(!c||evdev<1||evdev>247||(pressed!=0&&pressed!=1))return -1;if(!XTestFakeKeyEvent(c->d,evdev+8,pressed,CurrentTime))return -1;c->keys[evdev]=(unsigned char)pressed;XFlush(c->d);return 0;}
int nova_x11_pointer(struct nova_x11*c,int x,int y,unsigned button,int pressed){if(!c||x<0||y<0||(unsigned)x>=c->w||(unsigned)y>=c->h||button>5||(pressed!=0&&pressed!=1))return -1;if(!XTestFakeMotionEvent(c->d,c->screen,x,y,CurrentTime))return -1;if(button&&!XTestFakeButtonEvent(c->d,button,pressed,CurrentTime))return -1;if(button)c->buttons[button]=(unsigned char)pressed;XFlush(c->d);return 0;}

int nova_x11_request_close(struct nova_x11*c){if(!c)return -1;Window root,parent,*children=NULL;unsigned count=0;if(!XQueryTree(c->d,RootWindow(c->d,c->screen),&root,&parent,&children,&count))return -1;int sent=0;Atom protocols=XInternAtom(c->d,"WM_PROTOCOLS",False),del=XInternAtom(c->d,"WM_DELETE_WINDOW",False);for(unsigned i=0;i<count&&i<256;i++){XWindowAttributes a;if(!XGetWindowAttributes(c->d,children[i],&a)||a.map_state!=IsViewable)continue;Atom*list=NULL;int n=0;if(!XGetWMProtocols(c->d,children[i],&list,&n))continue;for(int k=0;k<n;k++)if(list[k]==del){XEvent e;memset(&e,0,sizeof e);e.xclient.type=ClientMessage;e.xclient.window=children[i];e.xclient.message_type=protocols;e.xclient.format=32;e.xclient.data.l[0]=(long)del;e.xclient.data.l[1]=CurrentTime;sent+=XSendEvent(c->d,children[i],False,NoEventMask,&e)!=0;break;}XFree(list);}if(children)XFree(children);XSync(c->d,False);return sent?0:-1;}
