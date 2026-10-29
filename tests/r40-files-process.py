#!/usr/bin/env python3
"""WSL real process + mock compositor test. NOT a NovaOS guest/real compositor test."""
import ctypes as C, os, socket, subprocess, tempfile, pathlib, re, array, mmap, sys, struct
if os.getuid()!=0:
 os.execvp("unshare",["unshare","-Ur","python3",__file__,"--root-ns"])
ROOT=pathlib.Path(__file__).resolve().parents[1]
task=pathlib.Path(tempfile.mkdtemp(prefix='nova-r16-'))
print('ARTIFACTS='+str(task),flush=True)
def abi(name):
 text=(ROOT/'src/runtime/nova_surface_protocol.h').read_text()
 body=text.split('struct '+name+' {')[1].split('};')[0]
 fields=[]
 for kind,field,size in re.findall(r'(unsigned long long|unsigned int|int|char)\s+(\w+)(?:\[([^]]+)\])?;',body):
  typ={'unsigned long long':C.c_ulonglong,'unsigned int':C.c_uint,'int':C.c_int,'char':C.c_char}[kind]
  if size:typ=typ*{'NOVA_SURFACE_TITLE_BYTES':32,'NOVA_SURFACE_CLIPBOARD_BYTES':64}[size]
  fields.append((field,typ))
 return type(name,(C.Structure,),{'_fields_':fields})
Message=abi('nova_surface_message');Event=abi('nova_surface_event')
assert C.sizeof(Message)==216 and C.sizeof(Event)==136
exe=task/'nova-files-process-test'
sources=['src/init/start.S','src/runtime/nova_runtime.c']+['files/src/'+x for x in ['nova_files_memory.c','nova_files_app.c','nova_files_core.c','nova_files_ui.c','nova_files_font.c','nova_files_surface_bridge.c']]
subprocess.run(['clang','-std=c11','-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-pic','-nostdlib','-static','-Os','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src/runtime'),'-I'+str(ROOT/'files/src'),'-DNOVA_FILES_SOCKET_PATH="'+str(task/'client.sock')+'"',*[str(ROOT/x) for x in sources],'-fuse-ld=lld','-Wl,-e,_start','-Wl,-z,noexecstack','-o',str(exe)],check=True)
# Without directory authority the app must fail before attempting socket access.
assert subprocess.run([exe],close_fds=True).returncode==21
root=task/'private';root.mkdir();(root/'Documents').mkdir();(root/'outside').symlink_to('/etc',target_is_directory=True)
for i in range(120):(root/('File-%03d.txt'%i)).write_text('real process fixture\n')
rootfd=os.open(root,os.O_RDONLY|os.O_DIRECTORY);assert rootfd==3
server=socket.socket(socket.AF_UNIX,socket.SOCK_SEQPACKET);server.bind(str(task/'client.sock'));server.listen(1);server.settimeout(5)
def run_cycle(oversized=False):
 def trusted_child_grant():
  tx,rx=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET)
  tx.sendmsg([struct.pack('4I',0x4e464447,1,3,1)],[(socket.SOL_SOCKET,socket.SCM_RIGHTS,array.array('i',[rootfd]))])
  os.close(rootfd);os.dup2(rx.fileno(),3);tx.close();rx.close()
 proc=subprocess.Popen([exe],pass_fds=(rootfd,),preexec_fn=trusted_child_grant)
 conn,_=server.accept();conn.settimeout(5)
 received_fd=None;ops=[];last_sequence=0
 def receive():
  nonlocal received_fd,last_sequence
  data,anc,flags,_=conn.recvmsg(4096,socket.CMSG_SPACE(4))
  assert len(data)==C.sizeof(Message) and not flags
  m=Message.from_buffer_copy(data)
  assert (m.magic,m.version,m.size,m.client_id)==(0x4e535246,6,216,3)
  assert m.sequence>last_sequence;last_sequence=m.sequence
  for level,kind,raw in anc:
   assert (level,kind)==(socket.SOL_SOCKET,socket.SCM_RIGHTS)
   values=array.array('i');values.frombytes(raw);assert len(values)==1
   assert received_fd is None;received_fd=values[0]
  ops.append(m.opcode);return m
 while receive().opcode!=6:pass
 assert ops==[1,8,2,10,11,12,4,5,6],ops
 assert received_fd is not None and os.fstat(received_fd).st_size==1600000
 pixels=mmap.mmap(received_fd,1600000,access=mmap.ACCESS_READ)
 before=pixels[:];assert any(before)
 def event(kind,**kw):
  e=Event();e.magic=0x4e535246;e.version=6;e.size=136;e.client_id=3;e.surface_id=300;e.type=kind
  for k,v in kw.items():setattr(e,k,v)
  conn.send(bytes(e))
 # Invalid client close must not terminate; valid configure must be acknowledged.
 if oversized:
  e=Event();e.magic=0x4e535246;e.version=6;e.size=136;e.client_id=3;e.surface_id=300;e.type=0x104
  conn.send(bytes(e)+b'extra')
  assert proc.wait(timeout=5)==22
  pixels.close();os.close(received_fd);conn.close();return
 event(0x104,client_id=2)
 event(0x103,x=80,y=100,width=800,height=500,configure_serial=7)
 assert receive().opcode==13
 assert receive().opcode==5 and receive().opcode==6
 # Native app page input crosses real v6 IPC; peer here is deliberately a mock.
 event(0x101,code=49,value=1)
 assert receive().opcode==5 and receive().opcode==6
 page_pixels=pixels[:];assert page_pixels!=before
 event(0x101,code=25,value=1)
 assert receive().opcode==5 and receive().opcode==6
 # Whole-folder filter must find an entry outside the first page.
 event(0x101,code=33,value=1,modifiers=2)
 assert receive().opcode==5 and receive().opcode==6
 for code in (33,23,38,18,12,2,2,10): # file-119 (outside first page)
  event(0x101,code=code,value=1)
  assert receive().opcode==5 and receive().opcode==6
 event(0x101,code=1,value=1)
 assert receive().opcode==5 and receive().opcode==6
 # A directory replaced by an escaping symlink must fail openat2 and PAINT
 # the real error. Keep all fixture objects; no accepted/user files are removed.
 saved=root/('Documents-original-'+str(proc.pid))
 (root/'Documents').rename(saved)
 (root/'Documents').symlink_to('/etc',target_is_directory=True)
 event(0x102,code=272,value=1,cursor_x=364,cursor_y=245)
 assert receive().opcode==5 and receive().opcode==6
 error_pixels=pixels[:];assert error_pixels!=before
 (root/'Documents').rename(root/('Documents-blocked-link-'+str(proc.pid)))
 saved.rename(root/'Documents')
 # First row is Documents; real openat2 + getdents64 redraws the directory.
 event(0x102,code=272,value=1,cursor_x=364,cursor_y=245)
 assert receive().opcode==5 and receive().opcode==6
 assert pixels[:]!=before
 event(0x104)
 assert receive().opcode==14
 m=receive();assert m.opcode==9 and m.buffer_id==1
 assert proc.wait(timeout=5)==0
 pixels.close();os.close(received_fd);conn.close()
run_cycle();run_cycle();run_cycle(oversized=True) # fresh process launch after graceful close
server.close();os.close(rootfd)
print('R40_PROCESS_OVERSIZED_PACKET_DENIED=PASS')
print('R40_PROCESS_MEMFD_SCM_RIGHTS=PASS')
print('R40_PROCESS_CONFIGURE_POINTER_DIRECTORY_NAVIGATION=PASS')
print('R40_PROCESS_WRONG_CLIENT_CLOSE_DENIED=PASS')
print('R40_PROCESS_CLOSE_AND_FRESH_REOPEN=PASS')
print('PAGING_WHOLE_FOLDER_FILTER_AND_ERROR_REDRAW=PASS REAL_VM_BOOT=NOT_RUN REAL_COMPOSITOR=NOT_RUN SANDBOX_ENFORCEMENT=NOT_TESTED')
