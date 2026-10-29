#ifndef NOVA_UI_PERMISSIONS_H
#define NOVA_UI_PERMISSIONS_H
/* POSIX mode formatting only; no access decision, ACL lookup or authority. */
static inline void nova_ui_mode_octal(unsigned mode,char out[5]){
 for(unsigned i=0;i<4;i++)out[i]=(char)('0'+((mode>>(9U-3U*i))&7U));out[4]=0;
}
static inline void nova_ui_permissions(unsigned mode,char out[10]){
 const char *bits="rwxrwxrwx";for(unsigned i=0;i<9;i++)out[i]=(mode&(1U<<(8U-i)))?bits[i]:'-';
 if(mode&04000)out[2]=(mode&0100)?'s':'S';
 if(mode&02000)out[5]=(mode&0010)?'s':'S';
 if(mode&01000)out[8]=(mode&0001)?'t':'T';out[9]=0;
}
static inline void nova_ui_unsigned(unsigned value,char out[11]){
 char reverse[10];unsigned n=0,i=0;do{reverse[n++]=(char)('0'+value%10U);value/=10U;}while(value);while(n)out[i++]=reverse[--n];out[i]=0;
}
#endif
