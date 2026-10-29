"""Bounded EFI partition sizing; preserves legacy 64 MiB for small kernels."""
import sys
from pathlib import Path
M=1024*1024

def esp_size(payload):
 if not isinstance(payload,int) or payload<=0 or payload>496*M:raise ValueError('EFI artifact outside bounded 496 MiB payload budget')
 return max(64*M,((payload+16*M+32*M-1)//(32*M))*(32*M))

if __name__=='__main__':print(esp_size(Path(sys.argv[1]).stat().st_size))
