#ifndef NOVA_PATH_DROP_H
#define NOVA_PATH_DROP_H
/* Fixed bounded data transfer. Not a file descriptor or execute capability. */
struct nova_path_drop {unsigned surface,down,accepted;int x,y;char path[96];};
static inline void nova_path_drop_cancel(struct nova_path_drop*d){d->surface=d->down=d->accepted=0;d->path[0]=0;}
static inline int nova_path_drop_offer(struct nova_path_drop*d,unsigned client,unsigned surface,unsigned committed,const char*s,unsigned n){
 if(client!=3||!committed||!d->down||d->accepted||!surface||surface!=d->surface||!n||n>=64||s[0]!='/'||s[n])return 0;
 unsigned component=1;for(unsigned i=1;i<=n;i++){unsigned char c=(unsigned char)s[i];if(i<n&&(c<32||c==127||c==92))return 0;if(c=='/'||i==n){unsigned len=i-component;if(!len||(len==1&&s[component]=='.')||(len==2&&s[component]=='.'&&s[component+1]=='.'))return 0;component=i+1;}}
 /* UTF8 scalar validation without locale or allocation. */
 for(unsigned i=0;i<n;){unsigned char c=(unsigned char)s[i];unsigned k=c<128?1:c>=194&&c<=223?2:c>=224&&c<=239?3:c>=240&&c<=244?4:0;if(!k||i+k>n)return 0;for(unsigned j=1;j<k;j++)if(((unsigned char)s[i+j]&192)!=128)return 0;if(k==3&&((c==224&&(unsigned char)s[i+1]<160)||(c==237&&(unsigned char)s[i+1]>=160)))return 0;if(k==4&&((c==240&&(unsigned char)s[i+1]<144)||(c==244&&(unsigned char)s[i+1]>=144)))return 0;i+=k;}
 const char*prefix="/home/nova";unsigned a=0;while(prefix[a]){d->path[a]=prefix[a];a++;}for(unsigned i=0;i<=n;i++)d->path[a+i]=s[i];d->accepted=1;return 1;
}
#endif
