#ifndef NOVA_HOME_DIRECTORY_PROTOCOL_H
#define NOVA_HOME_DIRECTORY_PROTOCOL_H
#include "nova_runtime.h"
#include "nova_files_grant.h"
#define NOVA_HOME_DIRECTORY_MAGIC 0x4e484442U
#define NOVA_HOME_DIRECTORY_VERSION 1U
#define NOVA_HOME_DIRECTORY_HANDLES 32U
#define NOVA_HOME_DIRECTORY_NAME 128U
#define NOVA_HOME_DIRECTORY_CAP_DETAILS 1U
/* STAMP reply.handle advertises optional read-only extensions. Old peers
 * return zero; old clients ignore this formerly unused field. */
#define NOVA_HOME_DIRECTORY_SHARED_ROOT "/run/nova/user-session/home"
enum {NOVA_HOME_DIRECTORY_OPEN=1,NOVA_HOME_DIRECTORY_META=2,NOVA_HOME_DIRECTORY_CLOSE=3,NOVA_HOME_DIRECTORY_STAMP=4,NOVA_HOME_DIRECTORY_PREVIEW=5,NOVA_HOME_DIRECTORY_DETAILS=6};
struct nova_home_directory_request {
 unsigned int magic,version,sequence,op,handle,reserved;
 char name[NOVA_HOME_DIRECTORY_NAME];
};
struct nova_home_directory_reply {
 unsigned int magic,version,sequence,op;
 int status;unsigned int handle;
 unsigned long long bytes;
 unsigned int kind,reserved;
};
/* Additive op 6 only. Legacy ops 1..5 retain their exact 40-byte replies.
 * Details are descriptive, not a read/write authorization decision. */
struct nova_home_directory_details_reply {
 struct nova_home_directory_reply base;
 unsigned int mode,uid,gid,links;
 long long modified_seconds;
};
_Static_assert(sizeof(struct nova_home_directory_details_reply)==64,"home details ABI");
_Static_assert(sizeof(struct nova_home_directory_request)==152,"home request ABI");
_Static_assert(sizeof(struct nova_home_directory_reply)==40,"home reply ABI");
static inline int nova_home_component_valid(const char *s){
 unsigned i=0;for(;i<NOVA_HOME_DIRECTORY_NAME&&s[i];i++)if(s[i]=='/'||s[i]=='\\'||(unsigned char)s[i]<32)return 0;
 if(!i||i==NOVA_HOME_DIRECTORY_NAME||(i==1&&s[0]=='.')||(i==2&&s[0]=='.'&&s[1]=='.'))return 0;
 for(;i<NOVA_HOME_DIRECTORY_NAME;i++)if(s[i])return 0;
 return 1;
}
#endif
