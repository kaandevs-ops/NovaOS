from pathlib import Path
import subprocess,sys,json,hashlib,shutil
r=Path(__file__).resolve().parents[1];a=sys.argv[1];assert a in ('x86_64','arm64')
subprocess.run([sys.executable,str(r/'scripts/build-r44-mupdf.py'),a],check=True)
cc='gcc' if a=='x86_64' else 'aarch64-linux-gnu-gcc'
libs=r/f'build/{"x86_64" if a=="x86_64" else "aarch64"}/mupdf';src=r/'build/deps/mupdf-1.28.5-source'
b=r/f'build/{a}/r25-terminal/System/Terminal/bin';assert b.is_dir()
common=[cc,'-std=c11','-Os','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fstack-protector-strong']
subprocess.run(common+[str(r/'src/pdf/nova_pdf.c'),'-o',str(b/'nova-pdf')],check=True)
subprocess.run(common+['-I'+str(src/'include'),str(r/'src/pdf/nova_pdf_worker.c'),str(libs/'libmupdf.a'),str(libs/'libmupdf-third.a'),'-Wl,--gc-sections','-lm','-o',str(b/'nova-pdf-worker')],check=True)
notice=b.parent/'Source/mupdf';notice.mkdir(parents=True,exist_ok=True)
for f in ('COPYING','README'):shutil.copy2(src/f,notice/f)
shutil.copy2(r/'third_party/mupdf/SOURCE.json',notice/'SOURCE.json')
for f in (r/'src/pdf').iterdir():
 if f.is_file():shutil.copy2(f,notice/f.name)
shutil.copy2(r/'scripts/build-r44-mupdf.py',notice/'build-r44-mupdf.py')
shutil.copy2(r/'scripts/build-r44-pdf.py',notice/'build-r44-pdf.py')
# The full corresponding source is shipped in the same offline portable ZIP,
# R51-Source-Review.zip -> third_party/mupdf.
# Keep the runtime's authenticated 40/48 MiB System geometry intact.
(notice/'SOURCE-AVAILABILITY.txt').write_text('MuPDF 1.28.5 AGPL v3 or later. Full corresponding source, worker and build recipes are included offline in this distribution: R51-Source-Review.zip, directory third_party/mupdf/. Source-host review also includes the same complete archive. Original archive URL/hash in SOURCE.json. No production signing keys are part of that source.\n')
# GPL/public-domain full sources are delivered offline with the VM SOURCE
# checkpoint, not duplicated in the tightly bounded authenticated runtime.
for rel in ('busybox-1.37.0.tar.bz2','sqlite/sqlite-amalgamation-3540100.zip'):
 original=b.parent/'Source'/rel
 if original.exists():
  external=r/'third_party/runtime-source'/rel;external.parent.mkdir(parents=True,exist_ok=True)
  if external.exists():assert hashlib.sha256(external.read_bytes()).digest()==hashlib.sha256(original.read_bytes()).digest()
  else:shutil.copy2(original,external)
  subprocess.run([sys.executable,str(r/'scripts/retain-build-files.py'),str(original)],check=True)
(b.parent/'Source/SOURCE-AVAILABILITY.txt').write_text('Full corresponding BusyBox and SQLite source archives are shipped offline in R51-Source-Review.zip -> third_party/runtime-source/. Configuration, patches and complete build scripts are in the same source checkpoint; COPYING notices remain here. MuPDF full source is in third_party/mupdf/. No signing keys are distributed.\n')
e=r/f'build/{a}/r44-pdf';e.mkdir(parents=True,exist_ok=True)
(e/'result.json').write_text(json.dumps({'arch':a,'status':'BUILD_PASS','guest':'PENDING','sha256':{f:hashlib.sha256((b/f).read_bytes()).hexdigest() for f in ('nova-pdf','nova-pdf-worker')}},indent=2))
print('R44_PDF_BUILD_PASS',a)
