from pathlib import Path
import json,os,tempfile,shutil,subprocess
r=Path(__file__).resolve().parents[1]
source=Path(json.loads((r/'evidence/r35/storage-growth-results.json').read_text())['path'])/'grown.ext4'
e=Path(tempfile.mkdtemp(prefix='workspace-mount-',dir=r/'evidence/r35'));data=e/'Data';(data/'Users').mkdir(parents=True)
image=data/'Users/nova-workspace.ext4';shutil.copyfile(source,image);image.chmod(0o600)
target=e/'home';target.mkdir()
c=e/'mount.c';c.write_text(r"""#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
#include <sched.h>
#include <grp.h>
#include "nova_terminal_workspace.h"
int main(int argc,char **argv){
 if(argc!=2)return 10;
 if(unshare(CLONE_NEWNS)||mount(NULL,"/",NULL,MS_REC|MS_PRIVATE,NULL))return 11;
 int image=workspace_open();if(image<0)return 12;
 if(workspace_mount(image,argv[1])){perror("workspace_mount");return 13;}
 close(image);
 struct statvfs v;if(statvfs(argv[1],&v)||(v.f_flag&(ST_NOEXEC|ST_NOSUID|ST_NODEV))!=(ST_NOEXEC|ST_NOSUID|ST_NODEV))return 14;
 if(chdir(argv[1]))return 15;
 int f=open("exec-probe",O_WRONLY|O_CREAT|O_EXCL,0755);if(f<0)return 16;
 const char script[]="#!/bin/sh\nexit 0\n";if(write(f,script,sizeof(script)-1)!=(ssize_t)(sizeof(script)-1))return 17;close(f);
 pid_t pid=fork();if(pid<0)return 18;
 if(!pid){
  if(setgroups(0,NULL)||setgid(12004)||setuid(12004))_exit(19);
  int user=open("user-mount-proof.txt",O_WRONLY|O_CREAT|O_EXCL,0600);if(user<0)_exit(20);
  if(write(user,"USER_WRITE_OK",13)!=13)_exit(21);close(user);
  char *a[]={"exec-probe",NULL};char *env[]={NULL};execve("exec-probe",a,env);
  _exit(errno==EACCES?0:22);
 }
 int status;if(waitpid(pid,&status,0)!=pid||status)return 23;
 struct stat s;if(stat("user-mount-proof.txt",&s)||s.st_uid!=12004||s.st_size!=13)return 24;
 if(chdir("/")||umount(argv[1]))return 25;
 puts("PRODUCTION_MOUNT_256MiB_USER_WRITE_NOEXEC_NOSUID_NODEV_PASS");return 0;
}
""")
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_WORKSPACE_DATA="{data}"','-I'+str(r/'src/terminal'),str(c),'-o',str(e/'mount')],check=True)
cp=subprocess.run([str(e/'mount'),str(target)],capture_output=True,text=True)
record={'status':'PASS' if cp.returncode==0 else 'FAIL','returncode':cp.returncode,'stdout':cp.stdout,'stderr':cp.stderr,'scope':'real production workspace_open/workspace_mount, loop device, private WSL mount namespace, UID12004 write, noexec/nosuid/nodev; not VM','path':str(e)}
(r/'evidence/r35/workspace-mount-results.json').write_text(json.dumps(record,indent=2));print(json.dumps(record,indent=2));assert cp.returncode==0
