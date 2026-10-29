from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];s=(root/'scripts/build-initramfs.sh').read_text()
start='if [[ "$STEP7_PROBES" == 1 ]]; then'
parts=s.split(start);assert len(parts)==3
checks=parts[1].split('\nif [[ "$NEED_USERLAND"')[0]
install=parts[2].split('\ncp "$ROOT/configs/recovery/')[0]
for mode,label in [(7,'idle-stress'),(6,'session-stress'),(5,'resource-probe'),(0,'resource-probe')]:
 task=Path(tempfile.mkdtemp(prefix='nova-r16-branches-'));(task/'System/Services').mkdir(parents=True)
 env=f'ROOT="{root}"; ROOTFS="{task}"; NEED_USERLAND=0; STEP7_PROBES={int(mode==7)}; STEP6_PROBES={int(mode==6)}; STEP5_PROBES={int(mode==5)}; PHASE18_ACCEPTANCE_PROBES=1\n'
 cp=subprocess.run(['bash','-eu','-c',env+start+checks+'\necho NEED=$NEED_USERLAND\n'],capture_output=True,text=True)
 assert cp.returncode==0 and 'NEED=1' in cp.stdout,(mode,cp.stderr)
 # Do not copy files or execute any build, trust, firmware, or signing code.
 cp=subprocess.run(['bash','-eu','-c',env+'cp(){ printf "COPY %s\\n" "$*"; };\n'+start+install],capture_output=True,text=True)
 assert cp.returncode==0,(mode,cp.stderr)
 lines=(task/'System/Services/services.list').read_text().splitlines()
 assert len(lines)==(4 if mode==5 else 2 if mode==0 else 1),(mode,lines)
 assert all(label in line for line in lines),(mode,lines)
 if mode==6:assert 'idle-stress' not in cp.stdout
 print(f'INITRAMFS_BRANCH_EXECUTION mode={mode} PASS')
# Negative fixture reproduces an unreachable Step-6 branch and must fail the same expectation.
mutated=(start+install).replace('elif [[ "$STEP6_PROBES" == 1 ]]; then','elif [[ "$STEP7_PROBES" == 1 ]]; then')
task=Path(tempfile.mkdtemp(prefix='nova-r16-bad-branch-'));(task/'System/Services').mkdir(parents=True)
env=f'ROOT="{root}"; ROOTFS="{task}"; STEP7_PROBES=0; STEP6_PROBES=1; STEP5_PROBES=0; PHASE18_ACCEPTANCE_PROBES=1\n'.replace('\\n','\n')
cp=subprocess.run(['bash','-eu','-c',env+'cp(){ :; };\n'.replace('\\n','\n')+mutated],capture_output=True,text=True)
assert cp.returncode==0,cp.stderr
lines=(task/'System/Services/services.list').read_text().splitlines()
assert lines!=['/System/Services/phase18-step6-session-stress.service']
assert all('session-stress' not in x for x in lines)
print('INITRAMFS_BRANCH_NEGATIVE_UNREACHABLE_STEP6_DETECTED=PASS')
