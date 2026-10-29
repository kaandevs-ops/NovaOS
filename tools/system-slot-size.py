# Experimental sizing MODEL ONLY; not enabled in signed System runtime.
"""Size only newly built verity System slots; no existing disks are resized."""
from pathlib import Path
import sys
M=1024*1024

def system_sizes(payload,inodes):
 if payload<0 or inodes<0:raise ValueError('invalid source accounting')
 needed=payload+inodes*512+16*M
 data=max(40,((needed+8*M-1)//(8*M))*8)
 if data>512:raise ValueError('System exceeds bounded 512 MiB filesystem budget')
 return data,data+8

def data_size(payload):
 if payload<0:raise ValueError('invalid Data payload')
 # Reserve the existing fixed 256 MiB user filesystem plus 64 MiB overhead.
 size=max(384,((payload+320*M+128*M-1)//(128*M))*128)
 if size>2048:raise ValueError('Data exceeds bounded 2 GiB image budget')
 return size

if __name__=='__main__':
 isdata=sys.argv[1]=='--data';root=Path(sys.argv[2] if isdata else sys.argv[1]);assert root.is_dir();payload=0;count=1
 for f in root.rglob('*'):
  st=f.lstat();count+=1
  if f.is_file() and not f.is_symlink():payload+=((st.st_size+4095)//4096)*4096
  else:payload+=4096
 print(data_size(payload) if isdata else " ".join(map(str,system_sizes(payload,count))))
