from pathlib import Path
import os,subprocess,json,struct,secrets,time,select,pwd,tempfile,socket,array,hashlib,fcntl
assert os.environ.get('NOVA_R47_PRIVATE_MOUNT')=='1', 'Run only inside explicit private mount namespace'
subprocess.run(['mount','-t','tmpfs','-o','mode=1777,nosuid,nodev,size=64m','nova-r47-test-tmp','/tmp'],check=True)
r=Path(__file__).resolve().parents[1];b=r/'build/browser-host';s=Path(tempfile.mkdtemp(prefix='browser-session-',dir=r/'evidence/r47'));s.chmod(0o700);uid=pwd.getpwnam('pc').pw_uid;gid=pwd.getpwnam('pc').pw_gid;os.chown(s,uid,gid)
# A mount-private /usr/bin overlay supplies xkbcomp without installing on host.
upper=s/'bin-upper';work=s/'bin-work';upper.mkdir();work.mkdir();import shutil
shutil.copy2(b/'sysroot/usr/bin/xkbcomp',upper/'xkbcomp')
subprocess.run(['mount','-t','overlay','overlay','-o',f'lowerdir=/usr/bin,upperdir={upper},workdir={work}','/usr/bin'],check=True)
Path('/tmp/.X11-unix').mkdir(mode=0o1777);os.chmod('/tmp/.X11-unix',0o1777)
profile=s/'profile';profile.mkdir(mode=0o700);os.chown(profile,uid,gid)
prefs='user_pref("browser.shell.checkDefaultBrowser", false);\nuser_pref("browser.aboutwelcome.enabled", false);\nuser_pref("browser.startup.homepage_override.mstone", "ignore");\nuser_pref("datareporting.healthreport.uploadEnabled", false);\nuser_pref("toolkit.telemetry.enabled", false);\n';(profile/'user.js').write_text(prefs);os.chown(profile/'user.js',uid,gid)
auth=s/'Xauthority';fields=[b'',b'',b'MIT-MAGIC-COOKIE-1',secrets.token_bytes(16)];auth.write_bytes(struct.pack('>H',65535)+b''.join(struct.pack('>H',len(f))+f for f in fields));auth.chmod(0o600);os.chown(auth,uid,gid)
def drop():os.setgroups([]);os.setgid(gid);os.setuid(uid)
runtime=s/'runtime';runtime.mkdir(mode=0o700);os.chown(runtime,uid,gid)
env={**os.environ,'XDG_RUNTIME_DIR':str(runtime),'HOME':str(s),'XAUTHORITY':str(auth),'LD_LIBRARY_PATH':str(b/'sysroot/usr/lib/x86_64-linux-gnu'),'MOZ_ENABLE_WAYLAND':'0'}
for name in ['MOZ_DISABLE_CONTENT_SANDBOX','MOZ_DISABLE_RDD_SANDBOX','MOZ_DISABLE_GPU_SANDBOX']:env.pop(name,None)
rd,wr=os.pipe();xf=(s/'xvfb.log').open('wb');x=subprocess.Popen([str(b/'sysroot/usr/bin/Xvfb'),'-displayfd',str(wr),'-screen','0','1024x768x24','-nolisten','tcp','-auth',str(auth),'-noreset'],env=env,preexec_fn=drop,pass_fds=(wr,),stdout=xf,stderr=xf);os.close(wr);assert select.select([rd],[],[],20)[0],(s/'xvfb.log').read_text();display=':'+os.read(rd,20).decode().strip();os.close(rd);assert x.poll() is None
env.pop('WAYLAND_DISPLAY',None);env.pop('DBUS_SESSION_BUS_ADDRESS',None);env['DISPLAY']=display;ff=(s/'firefox.log').open('wb');f=subprocess.Popen([str(b/'firefox/firefox'),'--no-remote','--profile',str(profile),'--new-window','https://www.google.com/','--width','1024','--height','768'],env=env,preexec_fn=drop,stdout=ff,stderr=ff,start_new_session=True)
(r/'evidence/r47/latest-browser-session.json').write_text(json.dumps({'path':str(s)}));(s/'session.json').write_text(json.dumps({'xvfb_pid':x.pid,'firefox_pid':f.pid,'display':display,'uid':uid,'sandbox_disable_flags':False,'scope':'WSL isolated X11 test only; NOT NovaOS guest'},indent=2))
for i in range(20):
 time.sleep(1)
 if f.poll() is not None:raise RuntimeError((s/'firefox.log').read_text())
subprocess.run([str(b/'x11-driver'),display,'capture',str(s/'google-first.ppm')],env=env,preexec_fn=drop,check=True)
# Fresh Firefox 158 first-run standard terms Continue, observed in this test profile.
subprocess.run([str(b/'x11-driver'),display,'click','512','326'],env=env,preexec_fn=drop,check=True)
time.sleep(3);assert f.poll() is None, 'Firefox exited after first-run Continue'
# Use the new native input adapter for address focus and Enter. xdotool supplies UTF8 typing only.
for code,down in [(29,1),(38,1),(38,0),(29,0)]:subprocess.run([str(b/'x11-driver'),display,'key',str(code),str(down)],env=env,preexec_fn=drop,check=True)
subprocess.run([str(b/'sysroot/usr/bin/xdotool'),'type','--clearmodifiers','--delay','15','https://www.google.com/search?q=Linux+file+permissions+chmod+chown'],env=env,preexec_fn=drop,check=True)
for down in [1,0]:subprocess.run([str(b/'x11-driver'),display,'key','28',str(down)],env=env,preexec_fn=drop,check=True)
time.sleep(12);subprocess.run([str(b/'x11-driver'),display,'capture',str(s/'google-search.ppm')],env=env,preexec_fn=drop,check=True)
assert f.poll() is None, 'Browser exited before search capture'
print('REAL_BROWSER_CAPTURE_READY',display,s,flush=True)
# Actual Firefox pixels -> native v6 metadata + real sealed memfd/SCM_RIGHTS.
a,bpeer=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET)
proc=subprocess.Popen([str(b/'frame-driver'),display,str(bpeer.fileno())],env=env,preexec_fn=drop,pass_fds=(bpeer.fileno(),),stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True);bpeer.close();a.settimeout(10);wire,control,flags,address=a.recvmsg(216,socket.CMSG_SPACE(4));assert len(wire)==216 and not flags
rights=array.array('i');assert len(control)==1 and control[0][0]==socket.SOL_SOCKET and control[0][1]==socket.SCM_RIGHTS;rights.frombytes(control[0][2]);assert len(rights)==1;received=rights[0];assert os.fstat(received).st_size==1024*768*4;assert fcntl.fcntl(received,fcntl.F_GET_SEALS)&fcntl.F_SEAL_SHRINK
raw=os.pread(received,1024*768*4,0);os.close(received);a.close();stdout,stderr=proc.communicate(timeout=10);assert proc.returncode==0,stderr;assert struct.unpack_from('<IIII',wire)==(0x4e535246,6,8,216)
from PIL import Image
Image.frombytes('RGB',(1024,768),raw,'raw','BGRX').save(s/'native-ipc-frame.png')
(s/'frame-transport.json').write_text(json.dumps({'status':'PASS','bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'client_id_metadata':4,'scope':'Real Firefox frame + native ABI + memfd/SCM_RIGHTS test peer; NOT authenticated compositor/guest','driver':stdout.strip()},indent=2))
# Close only this browser via normal ICCCM request, never destroy/kill window.
subprocess.run([str(b/'x11-driver'),display,'close'],env=env,preexec_fn=drop,check=True)
try:f.wait(timeout=15)
except subprocess.TimeoutExpired:raise RuntimeError('Own browser remains running; no force kill used')
x.terminate();x.wait(timeout=5)
(s/'completion.json').write_text(json.dumps({'browser_exit':f.returncode,'closed_via':'ICCCM WM_DELETE_WINDOW','scope':'host test only; visual review required'},indent=2))
