#!/usr/bin/env python3
"""Build verified libxcb sources for private runtime, never host installation.
The only behavior change removes abstract-X11 fallback, retaining filesystem
Unix X11 and ABI. All upstream copyright/license notices stay in the sources.
"""
from pathlib import Path
import os,subprocess,json,hashlib,difflib
p=Path(__file__).resolve().parents[1];b=p/'build/browser-host';src=b/'apt-source/libxcb-source';sysroot=b/'sysroot';out=b/'xcb-private-build';out.mkdir(exist_ok=True)
f=src/'src/xcb_util.c';old=f.read_text();new=old.replace('#ifdef HAVE_ABSTRACT_SOCKETS','#if defined(HAVE_ABSTRACT_SOCKETS) && !defined(NOVA_PRIVATE_X11_ONLY)')
if new!=old:
 assert old.count('#ifdef HAVE_ABSTRACT_SOCKETS')==3;f.write_text(new)
 (p/'evidence/r47/private-xcb.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='libxcb/src/xcb_util.c',tofile='libxcb/src/xcb_util.c')))
wrapper=out/'pkg-config'
wrapper.write_text('#!/bin/sh\ncase "$1" in\n --variable=xcbincludedir) echo "'+str(sysroot/'usr/share/xcb')+'";;\n --variable=pythondir) echo "'+str(sysroot/'usr/lib/python3/dist-packages')+'";;\n *) exec /usr/bin/pkg-config "$@";;\nesac\n');wrapper.chmod(0o755)
env={**os.environ,'PKG_CONFIG':str(wrapper),'PKG_CONFIG_LIBDIR':str(sysroot/'usr/lib/x86_64-linux-gnu/pkgconfig')+':'+str(sysroot/'usr/share/pkgconfig'),'PKG_CONFIG_SYSROOT_DIR':str(sysroot),'PYTHON':'/usr/bin/python3','PYTHONPATH':str(sysroot/'usr/lib/python3/dist-packages'),'CFLAGS':'-O2 -g -DNOVA_PRIVATE_X11_ONLY=1'}
with (out/'configure.log').open('w') as log:subprocess.run([str(src/'configure'),'--prefix=/usr','--libdir=/usr/lib/x86_64-linux-gnu','--disable-static','--enable-shared','--disable-devel-docs','--without-doxygen'],cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
with (out/'make.log').open('w') as log:subprocess.run(['make','-j2'],cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
# Only install into the already isolated local build sysroot.
with (out/'install.log').open('w') as log:subprocess.run(['make','install','DESTDIR='+str(sysroot)],cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
lib=sysroot/'usr/lib/x86_64-linux-gnu/libxcb.so.1'
report={'source':'verified Ubuntu libxcb 1.15-1ubuntu2','library':str(lib),'sha256':hashlib.sha256(lib.read_bytes()).hexdigest(),'patch':'private-xcb.patch','scope':'private guest runtime only; host libraries unchanged; runtime acceptance pending'}
(p/'evidence/r47/private-xcb-build.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
