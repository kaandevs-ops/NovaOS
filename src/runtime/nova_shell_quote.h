#ifndef NOVA_SHELL_QUOTE_H
#define NOVA_SHELL_QUOTE_H
/* Produces one literal shell argument; never adds a newline or submits it. */
static inline unsigned nova_shell_quote(const char *path,char *out,unsigned cap){
 unsigned n=0;if(!path||path[0]!='/'||cap<4)return 0;out[n++]=39;
 for(unsigned i=0;path[i];i++){unsigned char c=(unsigned char)path[i];if(i>=511||c<32||c==127)return 0;
 if(c==39){if(n+4>=cap)return 0;out[n++]=39;out[n++]=92;out[n++]=39;out[n++]=39;}
 else{if(n+1>=cap)return 0;out[n++]=(char)c;}}
 if(n+3>cap){return 0;}
 out[n++]=39;out[n++]=' ';out[n]=0;return n;
}
#endif
