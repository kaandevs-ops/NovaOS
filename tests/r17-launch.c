#define nova_main nova_init_unused
#include "../src/init/main.c"
#undef nova_main
#ifndef R17_MANIFEST
#error "fixed trusted test manifest required"
#endif
int nova_main(void){
 struct nova_service svc;
 if(parse_manifest(R17_MANIFEST,&svc)<0||!sandbox_prepare_persistent_data(&svc))return 29;
 if(!apply_application_sandbox(&svc))return 32;
 char *argv[]={svc.exec,0};char *envp[]={0};nova_execve(svc.exec,argv,envp);return 127;
}
