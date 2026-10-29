from pathlib import Path
import os,fcntl,resource,subprocess,tempfile,shutil,struct,json,time,hashlib,statistics
r=Path(__file__).resolve().parents[1];e=r/'evidence/r44/worker';e.mkdir(parents=True,exist_ok=True);d=Path(tempfile.mkdtemp(prefix='worker-',dir=e));d.chmod(0o755)
# A dedicated public traversal path, not chmod of any protected source tree.
public=Path(tempfile.mkdtemp(prefix='nova-r44-worker-'));public.chmod(0o755);worker=public/'worker';shutil.copy2(r/'build/x86_64/r25-terminal/System/Terminal/bin/nova-pdf-worker',worker);worker.chmod(0o755)
pdf=(r/'tests/fixtures/r44-two-pages.pdf').read_bytes();checks={};times=[];fd_before=len(os.listdir('/proc/self/fd'))
def run(data=pdf,args=('0','800','500','1000','0','0'),seal=True,mode=0o400):
 f=os.memfd_create('pdf',0x13);os.write(f,data);os.fchmod(f,mode)
 if seal:fcntl.fcntl(f,1033,0x2f)
 def child():
  if f!=3:os.dup2(f,3)
  os.setgroups([]);os.setgid(12004);os.setuid(12004);resource.setrlimit(resource.RLIMIT_AS,(32*1024*1024,)*2);resource.setrlimit(resource.RLIMIT_CPU,(120,120))
 start=time.monotonic();p=subprocess.run([str(worker),*args],pass_fds=(f,),preexec_fn=child,capture_output=True,timeout=8);elapsed=(time.monotonic()-start)*1000;os.close(f);return p,elapsed
for page in range(2):
 p,ms=run(args=(str(page),'800','500','1000','0','0'));assert p.returncode==0 and not p.stderr;h=struct.unpack('<13I',p.stdout[:52]);assert h[3]==page and h[4]==2 and len(p.stdout)==52+h[8];pixels=p.stdout[52:];w,height=h[5:7]
 def color(x,y):return tuple(pixels[(y*w+x)*3:(y*w+x)*3+3])
 banner=color(w//2,20);assert banner[2]>banner[1] if page==0 else banner[1]>banner[2]
 (d/f'page{page}.rgb').write_bytes(pixels);(d/f'page{page}.json').write_text(json.dumps(h));checks[f'real-page-{page}']='PASS';times.append(ms)
for name,kwargs in [('invalid-signature',{'data':b'not a PDF document'}),('truncated-pdf',{'data':b'%PDF-1.4 broken'}),('unsealed',{'seal':False}),('executable-mode',{'mode':0o500}),('oversize',{'data':b'%PDF-1.4'+b'0'*(4*1024*1024)}),('bad-page',{'args':('2','800','500','1000','0','0')}),('excess-width',{'args':('0','801','500','1000','0','0')}),('huge-number',{'args':('99999999999999999999','800','500','1000','0','0')})]:
 p,_=run(**kwargs);assert p.returncode!=0 and not p.stdout,(name,p.returncode,len(p.stdout));checks[name]='PASS'
for i in range(20):
 p,ms=run(args=(str(i%2),'800','500','1500','64','64'));assert p.returncode==0;times.append(ms)
assert len(os.listdir('/proc/self/fd'))==fd_before;checks['20-render-child-reap-FD-stability']='PASS'
report={'checks':checks,'status':'PASS','scope':'real isolated PDF process UID12004 AS32MiB; not VM','samples':len(times),'spawn-render-p50-ms':statistics.median(times),'spawn-render-max-ms':max(times),'source_sha256':hashlib.sha256(pdf).hexdigest(),'raw_ms':times,'path':str(d)}
(e/'result.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
