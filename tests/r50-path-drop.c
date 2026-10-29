#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "nova_path_drop.h"
#include "nova_surface_protocol.h"
_Static_assert(sizeof(struct nova_surface_message)==216,"IPC message ABI");
_Static_assert(sizeof(struct nova_surface_event)==136,"IPC event ABI");
int main(void){struct nova_path_drop d={.surface=300,.down=1};const char*s="/Demo/Türkçe belge.txt";assert(!nova_path_drop_offer(&d,2,300,1,s,strlen(s)));assert(!nova_path_drop_offer(&d,3,301,1,s,strlen(s)));assert(!nova_path_drop_offer(&d,3,300,0,s,strlen(s)));assert(nova_path_drop_offer(&d,3,300,1,s,strlen(s)));assert(!strcmp(d.path,"/home/nova/Demo/Türkçe belge.txt"));assert(!nova_path_drop_offer(&d,3,300,1,s,strlen(s)));nova_path_drop_cancel(&d);assert(!nova_path_drop_offer(&d,3,300,1,s,strlen(s)));
const char*bad[]={"relative","/../etc/passwd","/Demo/./x","/Demo//x","/Demo/","/x\ncommand","/x\033command","/x\\y","/\xc0\xaf","/\xed\xa0\x80"};for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++){d.surface=300;d.down=1;assert(!nova_path_drop_offer(&d,3,300,1,bad[i],strlen(bad[i])));}
char b[65];memset(b,'a',64);b[0]='/';b[64]=0;d.surface=300;d.down=1;assert(!nova_path_drop_offer(&d,3,300,1,b,64));assert(!nova_path_drop_offer(&d,3,300,1,b,~0U));puts("PASS gesture/client/surface/commit/UTF8/path bounds and unchanged v6 struct ABI");return 0;}
