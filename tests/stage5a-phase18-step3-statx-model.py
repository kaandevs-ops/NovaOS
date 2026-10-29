#!/usr/bin/env python3
"""Host Linux statx UAPI probe + pure policy cases; not guest QEMU evidence."""
import ctypes
import os
import platform
import stat
import tempfile
from pathlib import Path

AT_EMPTY_PATH = 0x1000
STATX_TYPE = 1
STATX_SIZE = 0x200
REGULAR = 0o100000
TYPE_MASK = 0o170000
MAX_BYTES = 65536

def allow(mask, mode, size):
    return mask & (STATX_TYPE | STATX_SIZE) == STATX_TYPE | STATX_SIZE and \
           mode & TYPE_MASK == REGULAR and size <= MAX_BYTES

assert allow(STATX_TYPE | STATX_SIZE, REGULAR | 0o444, 1)
assert allow(STATX_TYPE | STATX_SIZE, REGULAR | 0o444, MAX_BYTES)
assert not allow(STATX_TYPE, REGULAR | 0o444, 1)
assert not allow(STATX_SIZE, REGULAR | 0o444, 1)
assert not allow(STATX_TYPE | STATX_SIZE, stat.S_IFDIR | 0o755, 1)
assert not allow(STATX_TYPE | STATX_SIZE, stat.S_IFIFO | 0o600, 1)
assert not allow(STATX_TYPE | STATX_SIZE, REGULAR | 0o444, MAX_BYTES + 1)

arch = platform.machine()
if platform.system() != 'Linux' or arch not in ('x86_64', 'aarch64'):
    raise SystemExit('Requires Linux x86_64/aarch64 for host kernel statx ABI test')
SYS_STATX = 332 if arch == 'x86_64' else 291
libc = ctypes.CDLL(None, use_errno=True)
libc.syscall.restype = ctypes.c_long

def query(fd):
    data = ctypes.create_string_buffer(256)
    ret = libc.syscall(ctypes.c_long(SYS_STATX), ctypes.c_int(fd),
                       ctypes.c_char_p(b''), ctypes.c_int(AT_EMPTY_PATH),
                       ctypes.c_uint(STATX_TYPE | STATX_SIZE), ctypes.byref(data))
    if ret != 0:
        err = ctypes.get_errno()
        raise OSError(err, os.strerror(err))
    import struct
    mask = struct.unpack_from('=I', data, 0)[0]
    mode = struct.unpack_from('=H', data, 28)[0]
    size = struct.unpack_from('=Q', data, 40)[0]
    return mask, mode, size

with tempfile.TemporaryDirectory(prefix='nova-p18-step3-statx-') as td:
    base = Path(td)
    regular = base / 'regular'
    regular.write_bytes(b'nova')
    oversize = base / 'oversize'
    oversize.write_bytes(b'x' * (MAX_BYTES + 1))
    fifo = base / 'fifo'
    os.mkfifo(fifo)
    for path, expected in ((regular, True), (oversize, False), (base, False), (fifo, False)):
        flags = os.O_RDONLY | os.O_NONBLOCK | os.O_CLOEXEC | os.O_NOFOLLOW
        fd = os.open(path, flags)
        try:
            value = allow(*query(fd))
            assert value is expected, (path, value, expected)
        finally:
            os.close(fd)
print('Phase-18 Step-3 statx live host kernel / regular-directory-FIFO-oversize checks: PASS (not QEMU)')
