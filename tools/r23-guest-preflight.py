from pathlib import Path
import shutil,json,hashlib
r=Path(__file__).resolve().parents[1];assert r.name=='phase19-desktop-r23-integrated-evolution-candidate'
tools=['clang','ld.lld','cpio','python3','qemu-system-x86_64','swtpm','sbsign','mformat','mkfs.ext4','veritysetup']
firmware=[str(p) for root in ['/usr/share/OVMF','/usr/share/edk2'] if Path(root).is_dir() for p in Path(root).glob('**/*.fd')]
boot_sources={str(p.relative_to(r)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [r/'src/init/main.c',r/'scripts/build-r17-native.sh',r/'src/services/compositor.c',r/'src/runtime/nova_overview.h']}
report={'scope':'read-only discovery and source checks; no VM, image, key, TPM or firmware mutation','candidate':str(r.resolve()),'tools':{x:shutil.which(x) for x in tools},'firmware_candidates':firmware,'sources':boot_sources,'blocked_operations':['key generation or protected signing keys','GPT/ESP or signed image creation','VM creation/start','TPM/firmware configuration'],'pending':['select exact kernel and firmware with hashes after separate approval','isolated disk and test-key trust chain','real guest PID1 SO_PEERCRED endpoint','Data persistence and reboot tests','Secure Boot PCR dm-verity acceptance'],'protected_vm_targets':[]}
(r/'evidence/r23/guest-preflight.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
