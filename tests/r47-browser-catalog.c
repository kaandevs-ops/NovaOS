#include <assert.h>
#include <stdio.h>
#define nova_main unused_novad
#include "../src/init/main.c"
#undef nova_main
int main(void){struct nova_service s;int rc=parse_manifest("configs/services/nova-browser.service.pending",&s);
#if defined(NOVA_R47_BROWSER_COMPAT) && NOVA_R47_BROWSER_COMPAT
 assert(!rc&&application_sandbox_policy_valid(&s));assert(s.client_id==4&&!s.autostart&&!s.restart_always);
 assert(s.sandbox_profile==NOVA_APP_SANDBOX_PROFILE_BROWSER_V1&&s.sandbox_required_flags==NOVA_APP_SANDBOX_BROWSER_V1_FLAGS);
 struct nova_service bad=s;bad.client_id=3;assert(!application_sandbox_policy_valid(&bad));bad=s;copy_string(bad.app_id,sizeof(bad.app_id),"nova.fake");assert(!application_sandbox_policy_valid(&bad));
 bad=s;copy_string(bad.exec,sizeof(bad.exec),"/System/Apps/nova-demo-app");assert(!application_sandbox_policy_valid(&bad));bad=s;bad.declared_capabilities|=NOVA_APP_CAP_CLIPBOARD_READ;assert(!application_sandbox_policy_valid(&bad));
 bad=s;bad.sandbox_profile=NOVA_APP_SANDBOX_PROFILE_DESKTOP_V1;assert(!application_sandbox_policy_valid(&bad));bad=s;bad.sandbox_required_flags|=NOVA_APP_SANDBOX_FLAG_APP_DATA;assert(!application_sandbox_policy_valid(&bad));
 assert(INTEGRITY_BUFFER==131072);assert(NOVA_SYSTEM_SLOT_BYTES==520UL*1024*1024&&NOVA_VERITY_DATA_BLOCKS==131072&&NOVA_VERITY_HASH_START_BLOCK==131072);
 puts("PASS browser exact manifest/profile/capability binding and new bounded geometry (enabled mode)");
#else
 assert(rc!=0);assert(NOVA_SYSTEM_SLOT_BYTES==50331648&&NOVA_VERITY_DATA_BLOCKS==10240);puts("PASS disabled mode rejects browser profile, preserves legacy geometry");
#endif
 return 0;}
