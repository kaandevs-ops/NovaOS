from pathlib import Path
import subprocess,tempfile,os,json
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='gui-',dir=r/'evidence/r58'))
p=out/'user-session';p.mkdir(mode=0o700);h=p/'home';h.mkdir(mode=0o700);os.chown(h,12004,12004)
helper=out/'helper';bundle=r/'build/x86_64/r25-terminal/System/Terminal'
subprocess.run(['gcc','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_SHARED_WORKSPACE_PARENT="{p}"',f'-DNOVA_TERM_ROOT="{out}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_software_session.c'),'-o',str(helper)],check=True)
exe=out/'live';subprocess.run(['gcc','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation','-ffunction-sections','-fdata-sections','-Isrc/runtime',f'-DNOVA_SOFTWARE_HELPER="{helper}"','tests/r58-software-live.c','src/runtime/nova_runtime.c','-Wl,--gc-sections','-o',str(exe)],cwd=r,check=True)
x=subprocess.run([str(exe)],cwd=r,capture_output=True,text=True,timeout=120);(out/'live.log').write_text(x.stdout+x.stderr);print(out);print(x.stdout+x.stderr);assert x.returncode==0,x.returncode
