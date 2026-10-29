#!/usr/bin/env python3
"""Build isolated static command bundle and session helper; never signs or boots."""
from pathlib import Path
import subprocess,hashlib,tarfile,sys,shutil,json,os
r=Path(__file__).resolve().parents[1];arch=sys.argv[1] if len(sys.argv)>1 else 'x86_64'
assert arch in ('x86_64','arm64')
# Fixed helper allowlist must match the signed catalog before packaging.
catalog=json.loads((r/'configs/apps/local-packages.json').read_text())
assert [p['id'] for p in catalog['packages']]==['sqlite','lua','lua-dynamic','2048'], 'Software helper/catalog mismatch'
source=r/'third_party/busybox/busybox-1.37.0.tar.bz2'
assert hashlib.sha256(source.read_bytes()).hexdigest()=='3311dff32e746499f4df0d5df04d7eb396382d7e108bb9250e7b519b837043a4'
out=r/f'build/{arch}/r25-terminal';out.mkdir(parents=True,exist_ok=True)
build=out/'busybox-1.37.0'
if not build.exists():
 with tarfile.open(source) as t:
  for f in t.getmembers():
   assert (f.name=='busybox-1.37.0' or f.name.startswith('busybox-1.37.0/')) and '..' not in Path(f.name).parts and not f.issym() and not f.islnk()
  t.extractall(out,filter='data')
cc='gcc' if arch=='x86_64' else 'aarch64-linux-gnu-gcc'
log=(out/'build.log').open('w')
def run(cmd,cwd=build):
 p=subprocess.run(cmd,cwd=cwd,stdin=subprocess.DEVNULL,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'LC_ALL':'C'})
 if p.returncode:raise SystemExit(f'FAIL {cmd}; see {out}/build.log')
if not (build/'.nova-config-ready').exists() or (build/'.nova-config-ready').read_text()!='public-tools-v1-utf8':
 run(['make','allnoconfig'])
 enabled='UNICODE_SUPPORT BUSYBOX CAL FEATURE_SH_MATH FEATURE_SH_MATH_64 SHOW_USAGE FEATURE_VERBOSE_USAGE CHOWN FEATURE_CHOWN_LONG_OPTIONS STAT FEATURE_STAT_FORMAT PS FEATURE_SEAMLESS_GZ FIND FEATURE_FIND_TYPE FEATURE_FIND_MAXDEPTH FEATURE_FIND_PRINT0 XARGS FEATURE_XARGS_SUPPORT_ZERO_TERM DIFF FEATURE_DIFF_LONG_OPTIONS SHA256SUM FEATURE_MD5_SHA1_SUM_CHECK TAR FEATURE_TAR_CREATE GZIP GUNZIP ZCAT DU DF READLINK FEATURE_READLINK_FOLLOW REALPATH HEXDUMP BASE64 AWK FEATURE_AWK_LIBM CMP BASENAME DIRNAME VI FEATURE_VI_COLON FEATURE_VI_YANKMARK FEATURE_VI_SEARCH FEATURE_VI_USE_SIGNALS FEATURE_VI_DOT_CMD FEATURE_VI_UNDO FEATURE_VI_SETOPTS FEATURE_VI_SET FEATURE_VI_WIN_RESIZE STATIC FEATURE_EDITING FEATURE_TAB_COMPLETION FEATURE_EDITING_WINCH ASH SH_IS_ASH ASH_JOB_CONTROL ASH_ALIAS ASH_ECHO ASH_PRINTF ASH_TEST ASH_HELP ASH_CMDCMD ASH_OPTIMIZE_FOR_SIZE FEATURE_SH_STANDALONE FEATURE_PREFER_APPLETS ECHO PRINTF PWD LS FEATURE_LS_FILETYPES FEATURE_LS_TIMESTAMPS FEATURE_LS_USERNAME FEATURE_LS_SORTFILES FEATURE_LS_RECURSIVE CAT HEAD FEATURE_FANCY_HEAD TAIL WC SORT FEATURE_SORT_BIG GREP SED CUT TR UNAME ID WHOAMI DATE SLEEP TRUE FALSE TEST TEST1 TEST2 EXPR MKDIR RMDIR TOUCH CP MV RM CHMOD ENV PRINTENV STTY TTY YES SEQ CLEAR'.split()
 text=(build/'.config').read_text()
 for opt in enabled:
  old=f'# CONFIG_{opt} is not set'
  assert old in text or f'CONFIG_{opt}=y' in text,opt
  text=text.replace(old,f'CONFIG_{opt}=y')
 text=text.replace('CONFIG_FEATURE_EDITING_HISTORY=0','CONFIG_FEATURE_EDITING_HISTORY=32').replace('CONFIG_FEATURE_EDITING_MAX_LEN=0','CONFIG_FEATURE_EDITING_MAX_LEN=512')
 text=text.replace('CONFIG_SUBST_WCHAR=0','CONFIG_SUBST_WCHAR=63').replace('CONFIG_LAST_SUPPORTED_WCHAR=0','CONFIG_LAST_SUPPORTED_WCHAR=65535')
 text=text.replace('CONFIG_FEATURE_VI_MAX_LEN=0','CONFIG_FEATURE_VI_MAX_LEN=256')
 (build/'.config').write_text(text)
 run(['make','oldconfig'])
 (build/'.nova-config-ready').write_text('public-tools-v1-utf8')
run(['make','-j4',f'CC={cc}',f'CROSS_COMPILE={"" if arch=="x86_64" else "aarch64-linux-gnu-"}'])
root=out/'System/Terminal'
for d in ['bin','etc','Source']:(root/d).mkdir(parents=True,exist_ok=True)
shutil.copy2(build/'busybox',root/'bin/busybox');(root/'bin/busybox').chmod(0o755)
# External execvp (tar -> gzip, xargs -> command) needs real applet paths.
# Generate target configuration's list; never execute the ARM binary on host.
run(['make','busybox.links'])
for entry in (build/'busybox.links').read_text().splitlines():
 name=Path(entry).name
 assert name and name not in ('.','..','busybox','nova-info')
 dest=root/'bin'/name
 if dest.is_symlink():
  assert os.readlink(dest)=='busybox'
 elif dest.exists():
  raise SystemExit(f'Refusing applet overwrite: {dest}')
 else:dest.symlink_to('busybox')

(root/'etc/passwd').write_text('nova:x:12004:12004:Nova Terminal:/home/nova:/bin/busybox\n')
(root/'etc/group').write_text('nova:x:12004:\n')
(root/'etc/README.txt').write_text('Nova Terminal: real BusyBox ash and commands.\n/home/nova persists only when the terminal subtitle says Persistent workspace. Otherwise it is session-only. /tmp is always temporary.\nNo host filesystem or protected system access. Network and third-party repositories unavailable. Local verified catalog: nova-pkg list; install ID; upgrade ID; run ID [arguments].\nTry: pwd; id; uname -a; ls /; echo hello > hello.txt; cat hello.txt; printf "b\\na\\n" | sort\n')
shutil.copy2(source,root/'Source'/source.name);shutil.copy2(build/'LICENSE',root/'Source/LICENSE-BusyBox');shutil.copy2(build/'.config',root/'Source/busybox.config');shutil.copy2(r/'assets/fonts/terminal/COPYRIGHT-DejaVu.txt',root/'Source/COPYRIGHT-DejaVu.txt')
(root/'Source/README.txt').write_text('BusyBox 1.37.0 GPL-2.0-only source and exact configuration included.\nUpstream https://busybox.net/\nBuild recipe scripts/build-r25-terminal.py in NovaOS source.\nStatic libc: Ubuntu glibc toolchain. External distribution requires complete corresponding libc/relink materials and a security review; this is a local development bundle.\n')
info=root/'bin/nova-info'
run([cc,'-std=c11','-Os','-static','-Wall','-Wextra','-Werror','-fstack-protector-strong',str(r/'src/terminal/nova_info.c'),'-o',str(info)],r)
helper=out/'System/Core/nova-terminal-session';helper.parent.mkdir(parents=True,exist_ok=True)
run([cc,'-std=c11','-Os','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fstack-protector-strong',str(r/'src/terminal/nova_terminal_session.c'),'-o',str(helper)],r)
software=out/'System/Core/nova-software-session'
run([cc,'-std=c11','-Os','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fstack-protector-strong',str(r/'src/terminal/nova_software_session.c'),'-o',str(software)],r)
workspace=out/'System/Core/nova-user-workspace'
run([cc,'-std=c11','-Os','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fstack-protector-strong',str(r/'src/terminal/nova_user_workspace.c'),'-o',str(workspace)],r)
for f in [root/'bin/busybox',info,helper,workspace,software]:
 info=subprocess.check_output(['file',str(f)],text=True);assert 'statically linked' in info;print(info.strip())
log.close()
subprocess.run([sys.executable,str(r/'scripts/build-r29-sqlite.py'),arch],check=True)
(out/'result.json').write_text(json.dumps({'arch':arch,'status':'BUILD_PASS','source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'helper_sha256':hashlib.sha256(helper.read_bytes()).hexdigest(),'busybox_sha256':hashlib.sha256((root/'bin/busybox').read_bytes()).hexdigest(),'guest_boot':'PENDING'},indent=2))

subprocess.run([sys.executable,str(r/'scripts/build-r33-lua.py'),arch],check=True)
subprocess.run([sys.executable,str(r/'scripts/build-r36-runtime.py'),arch],check=True)
subprocess.run([sys.executable,str(r/'scripts/build-r55-game.py'),arch],check=True)
subprocess.run([sys.executable,str(r/'scripts/build-r32-pkg.py'),arch],check=True)

subprocess.run([sys.executable,str(r/'scripts/build-r37-network.py'),arch],check=True)


