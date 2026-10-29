#!/usr/bin/env python3
"""Run with WSL root; every mount is confined to a new private mount namespace."""
from pathlib import Path
import os,sys,subprocess,tempfile,shutil,json,socket,ctypes as C,re,array,mmap
r=Path(__file__).resolve().parents[1];assert os.getuid()==0,'Run in WSL as root; no VM access'
fixture=Path(tempfile.mkdtemp(prefix='sandbox-root.',dir=r/'build/x86_64/r16-integration'))
for name in ['System/Apps','Data/Nova/Apps/nova.files/Documents','run/nova/app','tmp']:(fixture/name).mkdir(parents=True,exist_ok=True)
for name in ['sandbox-test','sandbox-launch']:shutil.copy2(r/'build/x86_64/r16-integration'/name,fixture/name)
shutil.copy2(r/'build/x86_64/r16-files/nova-files',fixture/'System/Apps/nova-files')
data=fixture/'Data/Nova/Apps/nova.files';(data/'readme').write_text('original-data\n');(data/'escape').symlink_to('/System')
for p in [data,*data.rglob('*')]:os.chown(p,3,3,follow_symlinks=False)
# The chroot root must be a mountpoint for MS_REC|MS_PRIVATE inside production sandbox setup.
code='import os,subprocess,sys; p=sys.argv[1]; subprocess.run(["mount","--bind",p,p],check=True); os.chroot(p); os.chdir("/"); os.execv(sys.argv[2],[sys.argv[2]])'
def command(exe):return ['unshare','--mount','--propagation','private','python3','-c',code,str(fixture),'/'+exe]
cp=subprocess.run(command('sandbox-test'),capture_output=True,text=True,timeout=15)
assert cp.returncode==0,(cp.returncode,cp.stdout,cp.stderr)
print(cp.stdout.strip(),flush=True)
text=(r/'src/runtime/nova_surface_protocol.h').read_text()
def abi(name):
 body=text.split('struct '+name+' {')[1].split('};')[0];fields=[]
 for kind,field,size in re.findall(r'(unsigned long long|unsigned int|int|char)\s+(\w+)(?:\[([^]]+)\])?;',body):
  typ={'unsigned long long':C.c_ulonglong,'unsigned int':C.c_uint,'int':C.c_int,'char':C.c_char}[kind]
  if size:typ=typ*{'NOVA_SURFACE_TITLE_BYTES':32,'NOVA_SURFACE_CLIPBOARD_BYTES':64}[size]
  fields.append((field,typ))
 return type(name,(C.Structure,),{'_fields_':fields})
Message=abi('nova_surface_message');Event=abi('nova_surface_event')
s=socket.socket(socket.AF_UNIX,socket.SOCK_SEQPACKET)
os.chdir(fixture/'run/nova/app');s.bind('client.sock');os.chdir(r)
os.chmod(fixture/'run/nova/app/client.sock',0o666);s.listen(1);s.settimeout(8)
cycles=[]
for cycle in range(2):
 proc=subprocess.Popen(command('sandbox-launch'),stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 try:conn,_=s.accept()
 except Exception:
  print('launch exit',proc.poll());raise
 conn.settimeout(8);fd=None
 def recv():
  global fd
  payload,anc,flags,_=conn.recvmsg(4096,socket.CMSG_SPACE(4));assert len(payload)==216 and not flags
  for level,kind,raw in anc:
   assert (level,kind)==(socket.SOL_SOCKET,socket.SCM_RIGHTS)
   fds=array.array('i');fds.frombytes(raw);assert len(fds)==1;fd=fds[0]
  m=Message.from_buffer_copy(payload);assert m.client_id==3;return m
 while recv().opcode!=6:pass
 assert fd is not None;pixels=mmap.mmap(fd,1600000,access=mmap.ACCESS_READ);before=pixels[:]
 def event(kind,**kwargs):
  e=Event();e.magic=0x4e535246;e.version=6;e.size=136;e.client_id=3;e.surface_id=300;e.type=kind
  for k,v in kwargs.items():setattr(e,k,v)
  conn.send(bytes(e))
 event(0x103,x=80,y=100,width=800,height=500,configure_serial=9)
 assert recv().opcode==13 and recv().opcode==5 and recv().opcode==6
 event(0x102,code=272,value=1,cursor_x=364,cursor_y=245)
 assert recv().opcode==5 and recv().opcode==6;assert pixels[:]!=before
 event(0x101,code=14,value=1) # Backspace to parent through production key bridge
 assert recv().opcode==5 and recv().opcode==6
 event(0x104);assert recv().opcode==14
 m=recv();assert m.opcode==9 and m.buffer_id==1
 stdout,stderr=proc.communicate(timeout=8);assert proc.returncode==0,(proc.returncode,stderr)
 pixels.close();os.close(fd);conn.close();cycles.append('PASS')
s.close();assert (data/'readme').read_text()=='original-data\n'
for variant in ['source-symlink','writable-parent']:
 bad=Path(tempfile.mkdtemp(prefix='sandbox-negative.',dir=r/'build/x86_64/r16-integration'))
 for name in ['System/Apps','Data/Nova/Apps','run/nova/app','tmp']:(bad/name).mkdir(parents=True,exist_ok=True)
 shutil.copy2(r/'build/x86_64/r16-integration/sandbox-test',bad/'sandbox-test')
 shutil.copy2(r/'build/x86_64/r16-files/nova-files',bad/'System/Apps/nova-files')
 if variant=='source-symlink':(bad/'Data/Nova/Apps/nova.files').symlink_to('/System/Apps',target_is_directory=True)
 else:
  (bad/'Data/Nova/Apps/nova.files').mkdir();os.chmod(bad/'Data/Nova/Apps',0o777)
 cp=subprocess.run(['unshare','--mount','--propagation','private','python3','-c',code,str(bad),'/sandbox-test'],capture_output=True,text=True,timeout=15)
 assert cp.returncode==29,(variant,cp.returncode,cp.stderr)
 assert (bad/'System/Apps/nova-files').stat().st_uid==0
 print('R16_TRUSTED_SOURCE_DENIED='+variant,flush=True)
result={'kernel_sandbox':'PASS','production_files_under_actual_sandbox':cycles,'compositor':'mock surface peer; real compositor separately tested','VM':'NOT RUN','fixture':str(fixture)}
(r/'evidence/continuation/sandbox-runtime.json').write_text(json.dumps(result,indent=2))
print('R16_PRODUCTION_FILES_SANDBOX_POINTER_KEY_CLOSE_COLD_REOPEN=PASS',flush=True)
