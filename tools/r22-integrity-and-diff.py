from pathlib import Path
import json,hashlib,difflib
r=Path(__file__).resolve().parents[1];e=r/'evidence/r22';base=r.parent;r21=base/'phase19-desktop-r21-unified-evolution-candidate';r20=base/'phase19-desktop-r20-refinement-candidate';r19=base/'phase19-desktop-r19-premium-design-candidate';r18=base/'phase19-desktop-r18-visual-excellence-candidate';r17=base/'phase19-desktop-r17-evolution-candidate'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def verify(root,data):
 bad=[n for n,h in data.items() if not (root/n).is_file() or sha(root/n)!=h];return {'checked':len(data),'mismatches':bad,'status':'FAIL' if bad else 'PASS'}
b=json.loads((r/'r17-baseline.json').read_text())
checks={'protected483':verify(base,json.loads((r/'protected-sha256.json').read_text())),
'r16_original':verify(Path(b['r16_source']),b['r16_files']),'r16_frozen':verify(Path(b['frozen_reference']),b['r16_files']),
'r17_source_evidence':verify(r17,json.loads((r/'evidence/r18/r17-protected-baseline.json').read_text())),
'r17_native':verify(r17,json.loads((r17/'evidence/r17/native-binary-sha256.json').read_text())),
'r18_source_evidence':verify(r18,json.loads((r/'evidence/r19/r18-protected-baseline.json').read_text())),
'r18_native':verify(r18,{k:v['sha256'] for k,v in json.loads((r18/'evidence/r18/native-binary-verification.json').read_text()).items()}),
'r19_source_evidence':verify(r19,json.loads((r/'evidence/r20/r19-protected-baseline.json').read_text())),
'r19_native':verify(r19,{k:v['sha256'] for k,v in json.loads((r19/'evidence/r19/native-binary-verification.json').read_text()).items()}),
'r20_source_evidence':verify(r20,json.loads((r/'evidence/r21/r20-protected-baseline.json').read_text())),
'r20_native':verify(r20,{k:v['sha256'] for k,v in json.loads((r20/'evidence/r20/native-binary-verification.json').read_text()).items()}),
'r21_source_evidence':verify(r21,json.loads((e/'r21-protected-baseline.json').read_text())),
'r21_native':verify(r21,{k:v['sha256'] for k,v in json.loads((r21/'evidence/r21/native-binary-verification.json').read_text()).items()})}
(e/'integrity-final.json').write_text(json.dumps(checks,indent=2));assert all(v['status']=='PASS' for v in checks.values())
changes=[];diff=[];code=[]
for p in sorted(r.rglob('*')):
 if not p.is_file():continue
 rel=p.relative_to(r)
 if any(x in rel.parts for x in ['build','evidence','__pycache__']):continue
 q=r21/rel
 if q.is_file() and sha(q)==sha(p):continue
 changes.append({'path':str(rel),'status':'MODIFIED' if q.exists() else 'ADDED','before_sha256':sha(q) if q.is_file() else None,'after_sha256':sha(p)})
 try:
  lines=list(difflib.unified_diff(q.read_text().splitlines(True) if q.is_file() else [],p.read_text().splitlines(True),fromfile='R21/'+str(rel),tofile='R22/'+str(rel)));diff+=lines
  if p.suffix in ['.c','.h'] and (rel.parts[0]=='src' or rel.parts[:2]==('files','src')):code+=lines
 except UnicodeDecodeError:diff.append('Binary differs: '+str(rel)+'\n')
(e/'changed-files.json').write_text(json.dumps(changes,indent=2));(e/'source.diff').write_text(''.join(diff));(e/'native-code.diff').write_text(''.join(code));print(json.dumps(checks,indent=2));print('CHANGED',len(changes))
