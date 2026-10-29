#ifndef NOVA_HOME_FILE_H
#define NOVA_HOME_FILE_H
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
/* Capability is the pinned home root, not arbitrary host/cwd paths.
 * Flat names avoid mutable intermediate-directory traversal entirely. */
static inline int nova_text_name(const char*s){
 size_t n=strlen(s);if(!n||n>127||!strcmp(s,".")||!strcmp(s,"..")||!strncmp(s,".nova-edit-",11))return 0;
 for(size_t i=0;i<n;i++)if((unsigned char)s[i]<32||(unsigned char)s[i]>=127||s[i]=='/'||s[i]=='\\')return 0;return 1;
}
static inline int nova_text_root(int fd){struct stat st;if(fstat(fd,&st))return -1;if(!S_ISDIR(st.st_mode)||st.st_uid!=geteuid()||st.st_gid!=getegid()||(st.st_mode&07777)!=0700){errno=EPERM;return -1;}return 0;}
#endif
