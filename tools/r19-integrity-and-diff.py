#!/usr/bin/env python3
from pathlib import Path
import json,hashlib,difflib
r=Path(__file__).resolve().parents[1];e=r/'evidence/r19';old=r.parent/'phase19-desktop-r18-visual-excellence-candidate';r17root=r.parent/'phase19-desktop-r17-evolution-candidate'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def verify(root,manifest):
 bad=[n for n,h in manifest.items() if not (root/n).is_file() or sha(root/n)!=h]
 return {'checked':len(manifest),'mismatches':bad,'status':'FAIL' if bad else 'PASS'}
base=json.loads((r/'r17-baseline.json').read_text());r17=json.loads((r/'evidence/r18/r17-protected-baseline.json').read_text())
results={'r13_r14_r15':verify(r.parent,json.loads((r/'protected-sha256.json').read_text())),
 'r16_original':verify(Path(base['r16_source']),base['r16_files']), 'r16_frozen':verify(Path(base['frozen_reference']),base['r16_files']),
 'r17_source_and_evidence':verify(r17root,r17),'r17_recorded_native_binaries':verify(r17root,json.loads((r17root/'evidence/r17/native-binary-sha256.json').read_text())),
 'r18_source_and_evidence':verify(old,json.loads((e/'r18-protected-baseline.json').read_text())),
 'r18_recorded_native_binaries':verify(old,{k:v['sha256'] for k,v in json.loads((old/'evidence/r18/native-binary-verification.json').read_text()).items()})}
(e/'integrity-final.json').write_text(json.dumps(results,indent=2));assert all(x['status']=='PASS' for x in results.values())
changes=[];diff=[]
for p in sorted(r.rglob('*')):
 if not p.is_file():continue
 rel=p.relative_to(r)
 if any(x in rel.parts for x in ['build','evidence','__pycache__']):continue
 q=old/rel
 if q.is_file() and sha(q)==sha(p):continue
 changes.append({'path':str(rel),'status':'MODIFIED' if q.is_file() else 'ADDED','before_sha256':sha(q) if q.is_file() else None,'after_sha256':sha(p)})
 try:diff.extend(difflib.unified_diff(q.read_text().splitlines(True) if q.is_file() else [],p.read_text().splitlines(True),fromfile='R18/'+str(rel),tofile='R19/'+str(rel)))
 except UnicodeDecodeError:diff.append('Binary differs: '+str(rel)+'\n')
(e/'changed-files.json').write_text(json.dumps(changes,indent=2));(e/'source.diff').write_text(''.join(diff))
print(json.dumps(results,indent=2));print('Changed/added:',len(changes))
