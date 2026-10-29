from pathlib import Path
import subprocess,json,platform,time,sys
r=Path(__file__).resolve().parents[1];e=r/'evidence/r41';roots={'R40':r.parent/'phase19-desktop-r40-files-navigation-candidate','R41':r};runs={n:[] for n in roots}
for name,root in roots.items():
 cmd=['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'files/src'),'-I'+str(root/'src/runtime'),str(r/'tests/r41-properties-cost.c'),str(root/'files/src/nova_files_core.c'),str(root/'files/src/nova_files_font.c'),str(root/'src/runtime/nova_runtime.c'),str(root/'src/runtime/nova_font.c'),'-o',str(e/(name+'-properties-cost'))];subprocess.run(cmd,check=True)
for round in range(6):
 order=['R40','R41'] if round%2==0 else ['R41','R40']
 for name in order:runs[name].append(json.loads(subprocess.check_output([str(e/(name+'-properties-cost'))],text=True)))
record={'scope':'same host native Files properties 800x500, same real file and theme, O2, 20 warmup +200 render samples each run, six alternating pairs; NO VM/FPS/IPC latency claim','platform':platform.platform(),'background_load':Path('/proc/loadavg').read_text().strip(),'other_VM':'R40 VirtualBox running; shared host noise not controlled','runs':runs};(e/('properties-performance-idle-host.json' if '--idle-host' in sys.argv else 'properties-performance.json')).write_text(json.dumps(record,indent=2));print(json.dumps(record,indent=2))
