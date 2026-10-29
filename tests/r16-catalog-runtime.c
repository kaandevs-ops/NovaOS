#include <assert.h>
#include <stdio.h>
#define nova_main nova_init_unused
#include "../src/init/main.c"
#undef nova_main
int main(void){
 const char *paths[]={"configs/services/demo-app.service","configs/services/demo-app-b.service","configs/services/nova-files.service"};
 for(int i=0;i<3;i++){
  struct nova_service svc;assert(parse_manifest(paths[i],&svc)==0);
  assert(!service_registry_conflicts(&svc));copy_service(&g_services[g_service_count++],&svc);
 }
 assert(application_catalog_count()==3);
 struct nova_service *svc=find_application_by_id("nova.files");assert(svc&&svc->client_id==3);
 assert(find_application_by_client_id(3)==svc);
 assert(application_catalog_flags(svc)&NOVA_APP_CATALOG_FLAG_LAUNCHABLE);
 assert(service_registry_conflicts(svc));
 struct nova_service bad=*svc;bad.client_id=4;assert(!application_sandbox_policy_valid(&bad));
 bad=*svc;copy_string(bad.app_id,sizeof(bad.app_id),"nova.fake");assert(!application_sandbox_policy_valid(&bad));
 bad=*svc;copy_string(bad.exec,sizeof(bad.exec),"/System/Apps/nova-demo-app");assert(!application_sandbox_policy_valid(&bad));
 bad=*svc;bad.declared_capabilities|=NOVA_APP_CAP_CLIPBOARD_READ;assert(!application_sandbox_policy_valid(&bad));
 bad=*svc;bad.sandbox_profile=NOVA_APP_SANDBOX_PROFILE_DESKTOP_V1;assert(!application_sandbox_policy_valid(&bad));
 assert(svc->app_state==NOVA_APP_STATE_STOPPED&&!svc->autostart&&!svc->restart_always);
 puts("R16_ACTUAL_NOVAD_MANIFEST_CATALOG_ID_PROFILE_CAPABILITY_BINDING=PASS");return 0;
}
