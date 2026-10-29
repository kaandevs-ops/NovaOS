#ifndef NOVA_FILES_GUEST_METADATA_H
#define NOVA_FILES_GUEST_METADATA_H
#include "nova_files_guest_adapter.h"
#include "nova_files_metadata.h"
/* Existing Files-only syscall policy. O_PATH never opens file contents/devices.
 * Resolve one component beneath the already delegated current directory. */
static int nova_files_guest_metadata(int fd,const char*name,unsigned char type,struct nova_files_metadata*out){
 if(!out)return 0;unsigned char*clear=(unsigned char*)out;for(unsigned i=0;i<sizeof *out;i++)clear[i]=0;
 if(fd<0||!name||!name[0]||(type!=8&&type!=4))return 0;
 unsigned int n=0;for(;n<NOVA_FILES_NAME_BYTES&&name[n];n++)if(name[n]=='/'||name[n]=='\\'||(unsigned char)name[n]<32)return 0;
 if(!n||n==NOVA_FILES_NAME_BYTES||(n==1&&name[0]=='.')||(n==2&&name[0]=='.'&&name[1]=='.'))return 0;
 struct nova_files_open_how how={010000000UL|NOVA_O_CLOEXEC|NOVA_O_NOFOLLOW,0,NOVA_FILES_RESOLVE_BENEATH|NOVA_FILES_RESOLVE_NO_SYMLINKS|NOVA_FILES_RESOLVE_NO_MAGICLINKS};
 long handle=nova_syscall4(NOVA_FILES_NR_OPENAT2,fd,(long)name,(long)&how,sizeof(how));if(handle<0)return 0;
 struct nova_statx st;unsigned char*z=(unsigned char*)&st;for(unsigned int i=0;i<sizeof(st);i++)z[i]=0;
 long rc=nova_syscall5(NOVA_NR_statx,handle,(long)"",NOVA_AT_EMPTY_PATH,0x25f,(long)&st);nova_close((int)handle);
 unsigned int expected=type==8?0100000U:0040000U;
 if(rc<0||(st.stx_mask&NOVA_STATX_TYPE)==0||(st.stx_mode&0170000U)!=expected||(type==8&&(st.stx_mask&NOVA_STATX_SIZE)==0))return 0;
 out->valid=1;out->bytes=st.stx_size;
 if((st.stx_mask&0x1b)==0x1b){out->posix_valid=1;out->mode=st.stx_mode&07777;out->uid=st.stx_uid;out->gid=st.stx_gid;}return 1;
}
#endif
