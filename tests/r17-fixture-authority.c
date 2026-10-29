/* Host integration fixture. Uses production parser, PID registry and reply builder.
 * Does not claim normal_boot_policy_satisfied or firmware trust/PID1 acceptance. */
#define nova_main nova_init_unused
#include "../src/init/main.c"
#undef nova_main
int r17_fixture_register(const char *path,int pid){
 struct nova_service svc;
 if(g_service_count>=MAX_SERVICES||parse_manifest(path,&svc)<0||service_registry_conflicts(&svc))return 0;
 svc.pid=pid;svc.app_generation=1;svc.app_state=NOVA_APP_STATE_RUNNING;
 copy_service(&g_services[g_service_count++],&svc);return 1;
}
int r17_fixture_metadata(unsigned int client,char *display,unsigned int cap){
 struct nova_service *svc=find_application_by_client_id(client);if(!svc)return 0;
 copy_string(display,cap,svc->display_name);return 1;
}
void r17_fixture_authority(int fd,int control){
 struct nova_app_security_request req;
 for(;;){
  struct nova_pollfd pollfds[2]={{fd,NOVA_POLLIN|NOVA_POLLHUP,0},{control,NOVA_POLLIN|NOVA_POLLHUP,0}};
  struct nova_timespec timeout={1,0};if(nova_poll(pollfds,2,&timeout)<0)break;
  if(pollfds[1].revents&NOVA_POLLIN){
   int update[2];if(nova_read(control,update,sizeof(update))!=(long)sizeof(update))break;
   struct nova_service *svc=find_application_by_client_id((unsigned int)update[0]);
   if(!svc||update[1]<=0)break;svc->pid=update[1];svc->app_generation++;
   if(nova_write(control,update,sizeof(update))!=(long)sizeof(update))break;
  }
  if(pollfds[0].revents&NOVA_POLLIN){
   if(nova_read(fd,&req,sizeof(req))!=(long)sizeof(req))break;
  }else if(pollfds[0].revents&NOVA_POLLHUP)break;
  else continue;
  struct nova_app_security_reply reply;struct nova_service *svc=0;
  if(app_security_request_valid(&req)&&req.opcode==NOVA_APP_SECURITY_OP_LOOKUP_PID)
   svc=find_application_by_pid(req.process_id);
  if(svc&&application_capability_policy_valid(svc))
   fill_app_security_reply(&reply,&req,NOVA_APP_SECURITY_OP_PRINCIPAL,NOVA_APP_SECURITY_RESULT_OK,svc);
  else if(app_security_request_valid(&req)&&req.opcode==NOVA_APP_SECURITY_OP_HELLO)
   fill_app_security_reply(&reply,&req,NOVA_APP_SECURITY_OP_HELLO_REPLY,NOVA_APP_SECURITY_RESULT_OK,0);
  else fill_app_security_reply(&reply,&req,NOVA_APP_SECURITY_OP_ERROR,NOVA_APP_SECURITY_RESULT_NOT_FOUND,0);
  if(nova_write(fd,&reply,sizeof(reply))!=(long)sizeof(reply))break;
 }
}
