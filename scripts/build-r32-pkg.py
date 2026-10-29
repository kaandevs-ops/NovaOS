from pathlib import Path
import hashlib,subprocess,sys,json,struct
r=Path(__file__).resolve().parents[1];arch=sys.argv[1];assert arch in ('x86_64','arm64');cc='gcc' if arch=='x86_64' else 'aarch64-linux-gnu-gcc'
b=r/f'build/{arch}/r25-terminal/System/Terminal';(b/'bin').mkdir(parents=True,exist_ok=True)
out=r/f'build/{arch}/r33-catalog';out.mkdir(parents=True,exist_ok=True)
rows=[];records=[]
catalog=json.loads((r/'configs/apps/local-packages.json').read_text())
if catalog.get('source')!='local-system-catalog' or set(catalog)!={'format','source','packages'} or catalog.get('format')!=1 or not isinstance(catalog.get('packages'),list) or not 1<=len(catalog['packages'])<=9:raise SystemExit('Invalid catalog format or capacity (1..9)')
seen=set()
for item in catalog['packages']:
 if not isinstance(item,dict):raise SystemExit('Invalid catalog item')
 if set(item)!={'id','version','command','title','summary','category'}:raise SystemExit('Invalid catalog metadata')
 import re
 if any(not isinstance(v,str) or len(v)>160 or any(ord(c)<32 for c in v) for v in item.values()):raise SystemExit('Invalid catalog text')
 if not re.fullmatch(r'[a-z0-9][a-z0-9-]{0,47}',item['id']) or item['id'] in seen:raise SystemExit('Invalid/duplicate catalog id')
 if not re.fullmatch(r'[a-zA-Z0-9_-]{1,64}',item['command']):raise SystemExit('Invalid command basename')
 if item['category'] not in ('games','development','utilities'):raise SystemExit('Invalid category')
 if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._+-]{0,31}',item['version']):raise SystemExit('Invalid version')
 seen.add(item['id']);name=item['id'];version=item['version'];command=item['command']
 payload=b/'etc/nova-packages'/command
 if payload.is_symlink() or not payload.is_file() or payload.stat().st_size>8*1024*1024:raise SystemExit('Unsafe catalog payload')
 data=payload.read_bytes()
 if len(data)<64 or data[:6]!=b'\x7fELF\x02\x01' or struct.unpack_from('<H',data,18)[0]!={'x86_64':62,'arm64':183}[arch]:raise SystemExit('Payload ELF architecture mismatch')
 sha=hashlib.sha256(data).hexdigest()
 rows.append('{'+','.join(json.dumps(v) for v in [name,version,command,'/etc/nova-packages/'+command,sha,item['title'],item['summary'],item['category']])+'}')
 records.append({**item,'sha256':sha})
header=out/'catalog.h';header.write_text('static const struct nova_catalog_entry nova_catalog[]={'+','.join(rows)+'};\n')
cmd=[cc,'-Os','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',f'-DNOVA_PACKAGE_CATALOG="{header}"',str(r/'src/terminal/nova_pkg.c'),str(r/'src/runtime/nova_sha256.c'),'-o',str(b/'bin/nova-pkg')]
subprocess.run(cmd,check=True);manager=cmd.copy();manager[manager.index(str(r/'src/terminal/nova_pkg.c'))]=str(r/'src/terminal/nova_software.c');manager[-1]=str(b/'bin/nova-software');subprocess.run(manager,check=True);(out/'catalog.json').write_text(json.dumps(records,indent=2));print('MULTI_PACKAGE_CATALOG_BUILD_PASS',arch)
