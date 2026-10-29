from pathlib import Path
import subprocess,hashlib,json,tarfile,urllib.request
r=Path(__file__).resolve().parents[1];b=r/'build/browser-host';b.mkdir(parents=True,exist_ok=True);version='158.0.1';base=f'https://archive.mozilla.org/pub/firefox/releases/{version}/';primary='14F26682D0916CDD81E37B6D61B7B526D98F0353';subkey='827E658608679618CD349F93678E455D76767AA3'
class TLSRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,req,fp,code,msg,headers,newurl):
  if not newurl.startswith('https://'):raise ValueError('TLS downgrade rejected')
  return super().redirect_request(req,fp,code,msg,headers,newurl)
fetch=urllib.request.build_opener(TLSRedirect())
for name,url,limit in [('firefox.tar.xz',base+f'linux-x86_64/en-US/firefox-{version}.tar.xz',200*1024*1024),('SHA512SUMS',base+'SHA512SUMS',16*1024*1024),('SHA512SUMS.asc',base+'SHA512SUMS.asc',65536),('mozilla-key.asc','https://archive.mozilla.org/pub/firefox/nightly/latest-mozilla-central/KEY',1024*1024)]:
 f=b/name
 if not f.exists():
  with fetch.open(url,timeout=40) as resp,f.open('xb') as out:
   assert resp.geturl().startswith('https://');total=0
   while True:
    chunk=resp.read(1024*1024)
    if not chunk:break
    total+=len(chunk);assert total<=limit;out.write(chunk)
 assert f.stat().st_size<=limit
home=b/'gpg';home.mkdir(mode=0o700,exist_ok=True);subprocess.run(['gpg','--homedir',str(home),'--batch','--import',str(b/'mozilla-key.asc')],capture_output=True,check=True)
p=subprocess.run(['gpg','--homedir',str(home),'--batch','--status-fd','1','--verify',str(b/'SHA512SUMS.asc'),str(b/'SHA512SUMS')],capture_output=True,text=True,check=True)
valid=next(line.split() for line in p.stdout.splitlines() if line.startswith('[GNUPG:] VALIDSIG '));assert valid[2]==subkey and valid[-1]==primary;assert not any(tag in p.stdout for tag in ['BADSIG','REVKEYSIG','EXPKEYSIG'])
relative=f'linux-x86_64/en-US/firefox-{version}.tar.xz';expected=next(line.split()[0] for line in (b/'SHA512SUMS').read_text().splitlines() if line.split()[-1]==relative)
with (b/'firefox.tar.xz').open('rb') as f:actual=hashlib.file_digest(f,'sha512').hexdigest()
assert actual==expected
if not (b/'firefox').exists():
 with tarfile.open(b/'firefox.tar.xz') as t:t.extractall(b,filter='data')
(r/'evidence/r47/mozilla-signature.log').write_text(p.stdout+p.stderr)
(r/'evidence/r47/firefox-provenance.json').write_text(json.dumps({'version':version,'url':base,'sha512':actual,'signed_manifest':'VALIDSIG','primary_fingerprint':primary,'signing_subkey':subkey,'guest':'PENDING'},indent=2));print('MOZILLA_SIGNATURE_CURRENT_SUBKEY_SHA512_PASS',version)
