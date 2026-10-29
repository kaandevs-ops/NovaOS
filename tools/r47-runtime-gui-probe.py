#!/usr/bin/env python3
"""Start browser inside staged chroot. Host-only preparation, never guest proof."""
from pathlib import Path
import os,subprocess,json,time,struct,secrets,tempfile,select,shutil
P=Path(__file__).resolve().parents[1]
assert os.environ.get('NOVA_R47_PROBE')=='private'
root=Path(json.loads((P/'evidence/r47/runtime-stage.json').read_text())['runtime'])
session=Path(tempfile.mkdtemp(prefix='runtime-gui-',dir=P/'evidence/r47'));session.chmod(0o755)
subprocess.run(['mount','-t','tmpfs','-o','mode=1777,nosuid,nodev,size=64m','tmpfs',str(root/'tmp')],check=True)
(root/'tmp/.X11-unix').mkdir(mode=0o1777);(root/'tmp/.X11-unix').chmod(0o1777)
subprocess.run(['mount','-t','proc','-o','nosuid,nodev,noexec','proc',str(root/'proc')],check=True)
# Only explicit character devices; no host /dev tree or host home is exposed.
for name in ['null','zero','random','urandom']:
 dst=root/'dev'/name;dst.touch(exist_ok=True)
 subprocess.run(['mount','--bind','/dev/'+name,str(dst)],check=True)
(root/'dev/shm').mkdir(exist_ok=True)
subprocess.run(['mount','-t','tmpfs','-o','mode=1777,nosuid,nodev,noexec,size=64m','tmpfs',str(root/'dev/shm')],check=True)
home=root/'home/nova';home.chmod(0o700);os.chown(home,1000,1000)
for name in ['profile','runtime']:
 d=home/name;d.mkdir(exist_ok=True);d.chmod(0o700);os.chown(d,1000,1000)
auth=root/'tmp/auth';fields=[b'',b'',b'MIT-MAGIC-COOKIE-1',secrets.token_bytes(16)]
auth.write_bytes(struct.pack('>H',65535)+b''.join(struct.pack('>H',len(f))+f for f in fields));auth.chmod(0o600);os.chown(auth,1000,1000)
def enter():
 os.chroot(root);os.chdir('/');os.setgroups([]);os.setgid(1000);os.setuid(1000)
env={'PATH':'/usr/bin','HOME':'/home/nova','XDG_RUNTIME_DIR':'/home/nova/runtime','XAUTHORITY':'/tmp/auth','MOZ_ENABLE_WAYLAND':'0','LANG':'C.UTF-8'}
rd,wr=os.pipe();log=(session/'xvfb.log').open('wb')
x=subprocess.Popen(['/usr/bin/Xvfb','-displayfd',str(wr),'-screen','0','1024x768x24','-nolisten','tcp','-auth','/tmp/auth','-noreset'],env=env,preexec_fn=enter,pass_fds=(wr,),stdout=log,stderr=log);os.close(wr)
shutil.copyfile('/etc/resolv.conf',root/'etc/resolv.conf')
(root/'etc/hosts').write_text('127.0.0.1 localhost\n::1 localhost\n')
f=None;result={'scope':'host staged chroot; optional production compositor functions with fixture authority; no DRM/guest; host DNS/network'}
try:
 assert select.select([rd],[],[],15)[0], 'private X server readiness timeout'
 display=':'+os.read(rd,20).decode().strip();assert display!=':' and x.poll() is None
 env['DISPLAY']=display;flog=(session/'firefox.log').open('wb')
 f=subprocess.Popen(['/opt/firefox/firefox','--no-remote','--profile','/home/nova/profile','--new-window','https://www.google.com/search?q=Linux+chmod+chown'],env=env,preexec_fn=enter,stdout=flog,stderr=flog)
 time.sleep(12);result['browser_alive_after_12s']=f.poll() is None;result['browser_exit']=f.poll()
 # Capture uses explicit auth and this chroot's private socket directory via
 # a temporary mount-private bind, never WSLg's global socket directory.
 subprocess.run(['mount','-t','tmpfs','-o','mode=1777,nosuid,nodev,size=64m','tmpfs','/tmp'],check=True)
 Path('/tmp/.X11-unix').mkdir(mode=0o1777)
 subprocess.run(['mount','--bind',str(root/'tmp/.X11-unix'),'/tmp/.X11-unix'],check=True)
 henv={**os.environ,'DISPLAY':display,'XAUTHORITY':str(auth)}
 subprocess.run([str(P/'build/browser-host/x11-driver'),display,'click','461','326'],env=henv,check=True)
 time.sleep(12)
 r=subprocess.run([str(P/'build/browser-host/x11-driver'),display,'capture',str(session/'frame.ppm')],env=henv,capture_output=True,text=True,timeout=10)
 result['capture_exit']=r.returncode
 # Optional real production compositor adapter test (authority is fixture).
 if os.environ.get('NOVA_R47_NATIVE_TEST')=='1':
  for mode in ['spoof','unexpected-fd','disconnect']:
   neg=subprocess.run([str(P/'build/browser-host/browser-compositor-test'),display,str(session/(mode+'.ppm')),mode],env=henv,capture_output=True,text=True,timeout=25)
   (session/(mode+'.log')).write_text(neg.stdout+'\n'+neg.stderr)
   assert neg.returncode==0, neg.stderr[-2000:]
  result['native_negative_tests']=['spoof','unexpected-fd','disconnect']
  native=subprocess.run([str(P/'build/browser-host/browser-compositor-test'),display,str(session/'compositor-frame.ppm')],env=henv,capture_output=True,text=True,timeout=35)
  (session/'native-compositor.log').write_text(native.stdout+'\n'+native.stderr)
  result['native_compositor_exit']=native.returncode
  assert native.returncode==0, native.stderr[-2000:]
 # Request normal close, never force kill.
 subprocess.run([str(P/'build/browser-host/x11-driver'),display,'close'],env=henv,capture_output=True,timeout=10)
 if f.poll() is None:
  try:f.wait(timeout=15)
  except subprocess.TimeoutExpired:result['normal_close']='TIMEOUT'
 result['browser_exit']=f.poll()
finally:
 if f is not None and f.poll() is None and 'henv' in locals():
  subprocess.run([str(P/'build/browser-host/x11-driver'),display,'close'],env=henv,capture_output=True,timeout=10)
  try:f.wait(timeout=15)
  except subprocess.TimeoutExpired:pass
 os.close(rd)
 if f is not None and f.poll() is None:
  result['remaining_browser_pid']=f.pid
 else:
  x.terminate();x.wait(timeout=5)
 (session/'result.json').write_text(json.dumps(result,indent=2))
 print(session, json.dumps(result),flush=True)
