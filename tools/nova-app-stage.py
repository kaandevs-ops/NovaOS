#!/usr/bin/env python3
"""Offline native app staging for a NEW unsigned build directory, never live install."""
import argparse,hashlib,json,pathlib,re,struct

def stage(package,output,arch):
 package=package.resolve();metadata=package/'app.json'
 if metadata.is_symlink():raise ValueError('symlink manifest')
 if metadata.stat().st_size>8192:raise ValueError('manifest too large')
 m=json.loads(metadata.read_text());required={'id','name','client_id','binary','sha256','sandbox','capabilities'}
 if set(m)!=required:raise ValueError('unexpected or missing manifest field')
 if not isinstance(m['id'],str) or not re.fullmatch(r'[a-z][a-z0-9]*(?:[.][a-z0-9]+)+',m['id']) or len(m['id'])>48 or m['id'] in {'nova.files','nova.browser','nova.demo.a','nova.demo.b'}:raise ValueError('reserved or invalid app id')
 if type(m['client_id']) is not int or m['client_id'] not in (1,2):raise ValueError('only freed client slots 1/2 supported')
 if m['sandbox']!='desktop-v1':raise ValueError('unsupported sandbox')
 allowed={'ui.surface','input.text','input.tab-capture','clipboard.read','clipboard.write'}
 caps=m['capabilities']
 if not isinstance(caps,list) or not caps or len(set(caps))!=len(caps) or any(not isinstance(c,str) or c not in allowed for c in caps) or 'ui.surface' not in caps:raise ValueError('invalid capabilities')
 if not isinstance(m['name'],str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9 ._-]{0,47}',m['name']):raise ValueError('invalid display name')
 if not isinstance(m['binary'],str) or not re.fullmatch(r'[a-zA-Z0-9_-]{1,64}',m['binary']):raise ValueError('binary must be local basename')
 binary=package/m['binary']
 if binary.is_symlink() or not binary.is_file() or binary.stat().st_size>16777216:raise ValueError('invalid or oversized binary')
 data=binary.read_bytes()
 if hashlib.sha256(data).hexdigest()!=m['sha256']:raise ValueError('binary checksum mismatch')
 if len(data)<64 or data[:6]!=b'\x7fELF\x02\x01' or struct.unpack_from('<H',data,16)[0]!=2 or struct.unpack_from('<H',data,18)[0]!={'x86_64':62,'arm64':183}[arch]:raise ValueError('expected static architecture-matching ELF64 executable')
 off=struct.unpack_from('<Q',data,32)[0];size,count=struct.unpack_from('<HH',data,54)
 if size<56 or count>128 or off+size*count>len(data):raise ValueError('invalid program headers')
 if any(struct.unpack_from('<I',data,off+i*size)[0] in (2,3) for i in range(count)):raise ValueError('dynamic/interpreter ELF not supported')
 loads=[struct.unpack_from('<IIQQQQQQ',data,off+i*size) for i in range(count) if struct.unpack_from('<I',data,off+i*size)[0]==1]
 if not loads or not any(h[1]&1 for h in loads) or any(h[2]+h[5]>len(data) or h[5]>h[6] or h[1]&3==3 for h in loads):raise ValueError('invalid load segment or writable executable mapping')
 # No existing tree can be modified: caller integrates into signed build only after review.
 if output.exists():raise ValueError('output must be a new isolated staging directory')
 name=m['id'].replace('.','-');service='name='+name+'\nexec=/System/Apps/'+name+'\nrestart=never\ncritical=no\nenabled=yes\nkind=application\ndisplay_name='+m['name']+'\ncategory=productivity\ndescription=Reviewed native application\ncapabilities='+','.join(caps)+'\nsandbox=desktop-v1\napp_id='+m['id']+'\nclient_id='+str(m['client_id'])+'\nautostart=no\n'
 output.mkdir(parents=False)
 (output/name).write_bytes(data);(output/name).chmod(0o755);(output/(name+'.service')).write_text(service)
 result={'status':'STAGED_UNSIGNED','app':m['id'],'client_id':m['client_id'],'sha256':m['sha256'],'arch':arch,'requires':'catalog collision review, signed build integration, IPC/sandbox and real guest acceptance; not an installed app'}
 (output/'stage-result.json').write_text(json.dumps(result,indent=2));return result

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('package',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--arch',choices=['x86_64','arm64'],required=True);a=p.parse_args()
 try:print(json.dumps(stage(a.package,a.output,a.arch)))
 except (ValueError,OSError,TypeError,KeyError) as e:p.exit(2,'REJECTED: '+str(e)+'\n')
