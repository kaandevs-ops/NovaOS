#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#define nova_main unused_init_entry
#include "../src/init/main.c"
#undef nova_main
int main(int argc,char**argv){assert(argc==3);int want=atoi(argv[2]);struct nova_service s;int rc=parse_manifest(argv[1],&s);if(want<0)assert(rc<0);else{assert(rc==0&&s.files_user_home==want);struct nova_service copy;nova_zero(&copy,sizeof copy);copy_service(&copy,&s);assert(copy.files_user_home==want);if(want){copy.sandbox_profile=NOVA_APP_SANDBOX_PROFILE_DESKTOP_V1;assert(!application_sandbox_policy_valid(&copy));}}puts("PASS: signed manifest home scope policy");}
