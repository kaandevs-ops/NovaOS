#define nova_main nova_init_unused
#include "../src/init/main.c"
#undef nova_main
/* argv-free harness: driver mounts fixture as /tmp/nova-r16-root through chroot(1).
 * sandbox is entered before any directory authority is received. */
static void check(int ok,int code){if(!ok)nova_exit(code);}
int nova_main(void){
 struct nova_service svc;nova_zero(&svc,sizeof(svc));svc.is_application=1;svc.client_id=3;
 copy_string(svc.name,sizeof(svc.name),"nova-files");copy_string(svc.app_id,sizeof(svc.app_id),"nova.files");
 copy_string(svc.exec,sizeof(svc.exec),"/System/Apps/nova-files");
 svc.declared_capabilities=svc.effective_capabilities=NOVA_APP_CAP_UI_SURFACE|NOVA_APP_CAP_INPUT_TEXT;
 svc.sandbox_profile=NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1;svc.sandbox_required_flags=NOVA_APP_SANDBOX_FILES_READ_V1_FLAGS;
 check(sandbox_prepare_persistent_data(&svc),29);
 check(nova_unshare(NOVA_CLONE_NEWNS)>=0,30);
 check(nova_mount(0,"/",0,NOVA_MS_REC|NOVA_MS_PRIVATE,0)>=0,31);
 check(apply_application_sandbox(&svc),32);
#ifdef NOVA_R16_EXEC_FILES
 char *argv[]={svc.exec,0};char *envp[]={0};nova_execve(svc.exec,argv,envp);nova_exit(45);
#endif
 int fd=nova_files_receive_grant(3);nova_close(3);check(fd>=0,33);
 check(nova_files_directory_fd_valid(fd),34);
 struct {unsigned long long flags,mode,resolve;} how={0200000,0,0x0e};
 check(nova_syscall4(NOVA_FILES_NR_OPENAT2,fd,(long)"Documents",(long)&how,sizeof(how))>=0,35);
 check(nova_syscall4(NOVA_FILES_NR_OPENAT2,fd,(long)"escape",(long)&how,sizeof(how))<0,36);
 check(nova_syscall4(NOVA_FILES_NR_OPENAT2,fd,(long)"..",(long)&how,sizeof(how))<0,37);
 how.flags=NOVA_O_WRONLY;
 check(nova_syscall4(NOVA_FILES_NR_OPENAT2,fd,(long)"readme",(long)&how,sizeof(how))<0,38);
 check(nova_syscall4(NOVA_NR_openat,fd,(long)"readme",NOVA_O_WRONLY,0)==-NOVA_EPERM,39);
 check(nova_syscall3(NOVA_NR_unlinkat,fd,(long)"readme",0)==-NOVA_EPERM,40);
 check(nova_unshare(NOVA_CLONE_NEWNS)==-NOVA_EPERM,41);
 check(nova_syscall3(NOVA_FILES_NR_FCNTL,fd,4,0)==-NOVA_EPERM,46);
 check(nova_socket(2,1,0)==-NOVA_EPERM,42);
 /* A shared writable memfd is required for compositor pixels and remains allowed. */
 long mem=nova_memfd_create("test",NOVA_MFD_CLOEXEC);check(mem>=0,43);
 check(nova_ftruncate((int)mem,4096)==0,44);nova_close((int)mem);nova_close(fd);
 nova_putln("R16_ACTUAL_NOVAD_NAMESPACE_RO_GRANT_SECCOMP=PASS");return 0;
}
