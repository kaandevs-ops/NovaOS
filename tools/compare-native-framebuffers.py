#!/usr/bin/env python3
"""Lossless side-by-side native P6 framebuffer comparison; no resampling or retouching."""
from pathlib import Path
import sys,json,hashlib,subprocess
left,right,out=map(Path,sys.argv[1:4])
def read(path):
 data=path.read_bytes();header,rgb=data.split(b"\n255\n",1);lines=header.splitlines();assert lines[0]==b"P6";w,h=map(int,lines[1].split());assert len(rgb)==w*h*3;return w,h,rgb
w,h,a=read(left);ww,hh,b=read(right);assert (w,h)==(ww,hh)==(1600,900)
pixels=b"".join(a[y*w*3:(y+1)*w*3]+b[y*w*3:(y+1)*w*3] for y in range(h))
ppm=out.with_suffix('.ppm');ppm.write_bytes(f"P6\n{w*2} {h}\n255\n".encode()+pixels)
subprocess.run([sys.executable,str(Path(__file__).with_name('export-native-framebuffer.py')),str(ppm),str(out)],check=True)
meta={'scope':'native host renderer, NOT VirtualBox','left':'unchanged protected R17 production sources/binaries, deterministic comparison harness built in R18','right':'R18 native code','resolution_each':[w,h],'same_scene':'same 5 surfaces, positions, photo and filesystem fixture; application pixel style differs','resampled':False,'left_sha256':hashlib.sha256(left.read_bytes()).hexdigest(),'right_sha256':hashlib.sha256(right.read_bytes()).hexdigest(),'changed_rgb_bytes':sum(x!=y for x,y in zip(a,b))}
out.with_suffix('.json').write_text(json.dumps(meta,indent=2));print(json.dumps(meta,indent=2))
