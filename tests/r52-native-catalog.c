#include <assert.h>
#include <stdio.h>
#define nova_main unused_novad
#include "../src/init/main.c"
#undef nova_main
int main(int argc,char**argv){assert(argc==2);struct nova_service s;assert(parse_manifest(argv[1],&s)==0);assert(application_sandbox_policy_valid(&s));assert(s.client_id==1);assert(s.sandbox_profile==NOVA_APP_SANDBOX_PROFILE_DESKTOP_V1);assert(!s.autostart);assert(!service_registry_conflicts(&s));copy_service(&g_services[g_service_count++],&s);assert(service_registry_conflicts(&s));puts("PASS actual novad parses staged manifest; duplicate identity rejected");}
