#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "../files/src/nova_files_guest_adapter.h"
#include "../files/src/nova_files_guest_metadata.h"
#include "../files/src/nova_files_home_client.h"
int main(void){
 union{struct nova_cmsghdr a;unsigned char b[NOVA_CMSG_SPACE(4)+NOVA_CMSG_SPACE(sizeof(int))];}control;
 nova_zero(&control,sizeof control);struct nova_cmsghdr*unknown=(void*)control.b;unknown->cmsg_len=NOVA_CMSG_LEN(4);unknown->cmsg_level=NOVA_SOL_SOCKET;unknown->cmsg_type=99;
 int original=open("/dev/null",O_RDONLY);assert(original>=0);int passed=dup(original);assert(passed>=0);struct nova_cmsghdr*rights=(void*)(control.b+NOVA_CMSG_SPACE(4));rights->cmsg_len=NOVA_CMSG_LEN(sizeof(int));rights->cmsg_level=NOVA_SOL_SOCKET;rights->cmsg_type=NOVA_SCM_RIGHTS;nova_copy((unsigned char*)rights+NOVA_CMSG_DATA_OFFSET,&passed,sizeof passed);
 struct nova_msghdr msg;nova_zero(&msg,sizeof msg);msg.msg_controllen=sizeof control;int fds[4],valid=1;unsigned count=home_receive_rights(&msg,control.b,fds,4,&valid);assert(!valid&&count==1&&fds[0]==passed);for(unsigned i=0;i<count;i++)nova_close(fds[i]);assert(fcntl(passed,F_GETFD)<0);close(original);
 char name[128]={0};assert(!nova_home_component_valid(name));name[0]='.';assert(!nova_home_component_valid(name));name[1]='.';assert(!nova_home_component_valid(name));nova_zero(name,sizeof name);name[0]='a';assert(nova_home_component_valid(name));name[2]='x';assert(!nova_home_component_valid(name));nova_zero(name,sizeof name);name[0]='a';name[1]='/';assert(!nova_home_component_valid(name));puts("PASS: unknown ancillary followed by rights does not leak FDs; canonical bounded component validation");
}
