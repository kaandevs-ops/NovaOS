#!/usr/bin/env python3
"""Lossless PPM -> PNG framebuffer export, no visual editing or generated assets."""
from pathlib import Path
import sys,struct,zlib
source=Path(sys.argv[1]);target=Path(sys.argv[2])
with source.open('rb') as f:
 assert f.readline().strip()==b'P6';w,h=map(int,f.readline().split());assert f.readline().strip()==b'255';pixels=f.read()
assert len(pixels)==w*h*3
raw=b''.join(b'\0'+pixels[y*w*3:(y+1)*w*3] for y in range(h))
def chunk(kind,data):return struct.pack('!I',len(data))+kind+data+struct.pack('!I',zlib.crc32(kind+data)&0xffffffff)
target.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('!2I5B',w,h,8,2,0,0,0))+chunk(b'tEXt',b'Description\0Native NovaOS host framebuffer; real app surfaces; NOT A VM SCREENSHOT')+chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))
print(str(target))
