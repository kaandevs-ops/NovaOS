from pathlib import Path
import json,subprocess
r=Path(__file__).resolve().parents[1];e=r/'evidence/r23'
a=json.loads((e/'acceptance-results.json').read_text());perf=json.loads((e/'performance-study/summary.json').read_text());soak=json.loads((e/'soak.json').read_text());native=json.loads((e/'native-binary-verification.json').read_text());integrity=json.loads((e/'integrity-final.json').read_text());visual=json.loads((e/'visual-matrix.json').read_text());cost=json.loads((e/'feature-cost.json').read_text());build=subprocess.check_output(['python3','tools/r23-build-id.py'],cwd=r,text=True).strip()
soak_brief={'cycles':soak['cycles'],'samples':len(soak['samples']),'elapsed_ms':soak['samples'][-1]['elapsed_ms'],'parent_rss_range_kib':[min(x['parent_rss_kib'] for x in soak['samples']),max(x['parent_rss_kib'] for x in soak['samples'])],'app_rss_last_kib':soak['samples'][-1]['app_rss_kib'],'all_sampled_fds_unchanged':all(x['fds_unchanged'] for x in soak['samples'])}
rows='\n'.join('| '+v+' | '+str(round(d['pooled_p50_ms'],3))+' | '+str(round(d['pooled_p95_ms'],3))+' | '+str(d['peak_rss_kib'])+' |' for v,d in perf['summary'].items())
checks='\n'.join('- '+k+': '+v['status'] for k,v in a['groups'].items())
text=f"""# NovaOS R23 teknik sonuç raporu

## Devralınan R22 ve izolasyon

R22 adayındaki gerçek rapor/test kaydı 40 PASS/0 FAIL, oturum bazlı ayarlar ve gerçek guest PENDING durumuyla doğrulandı. 1901 R22 kaynak/kanıt kaydı başlangıç SHA-256 envanterine alındı; R23 ayrı kopyada geliştirildi: `{r}`. R22 yerinde değiştirilmedi. Önceki p50/p95 değerleri tarihsel ölçümdür; aşağıdaki yeni çalışma ile karıştırılmadı.

Build kaynak kimliği: `{build}`. NOVA_VERSION mevcut 0.0.12-dev değeridir; sahte yeni çekirdek sürümü üretilmedi.

## SOURCE IMPLEMENTED

- `src/init/main.c`: yalnızca denetlenen compositor peer'ine hizmet veren bounded read-only Overview listener, servis/uygulama/restart ve mevcut boot durum özetleri; nonblocking, CLOEXEC, bozuk paket/ancillary reddi, hata kapanışı. PID 1 event loop'una bağlıdır.
- `src/runtime/nova_overview.h`: ayrı sürüm 1 telemetri protokolü; katı uzunluk/sequence/reserved/count kontrolleri. Surface v6 ABI değiştirilmedi.
- `src/services/overview_client.inc`, `src/services/compositor.c`: gerçek Settings Overview sayfası, güvenilir PID 1 istemcisi, timeout/rate limit/disconnect; Quick Settings Overview eylemi, sidebar, F1 ve Ctrl+Alt+I erişimi, R yenileme. Dördüncü bağımsız uygulama veya kapasite artışı yok.
- `src/runtime/nova_build_info.h`, `tools/r23-build-id.py`, build scriptleri: gerçek kaynak hash'i, sürüm ve mimari gösterimi.
- `files/src/nova_files_ui.c`, `files/src/nova_files_surface_bridge.c`: Size başlığı/B ile artan/azalan gerçek byte sıralaması, seçim korunması, seçili dosyanın gerçek boyut bilgisi. Bilinmeyen/dizin boyutu '-' ve son sırada; salt okunur model korunur.
- `src/runtime/nova_preferences_store.h`: broker'a bağlanmamış sürümlü atomik tercih depolama bileşeni. Sahiplik/mod/path/symlink/generation/fsync kontrolleri ve belirsiz commit sonucu. Gerçek kullanıcı oturumu/broker eksik; UI **Session only** kalır.

İncelenen kaynaklar: yukarıdakiler yanında shell.c katalog doğrulaması, demo_app.c tema/configure ACK, Files core/metadata, uygulama katalog/principal/sandbox/surface protokolleri ve unsigned rootfs build zinciri. Shell ve Demo A/B kaynakları yeniden yazılmadı.

## FUNCTIONAL / SECURITY

Son kabul: **{a['PASS']} PASS, {a['FAIL']} FAIL**. İlk geliştirme sırasında yanlış firmware enum adı üretim syntax kontrolünde düzeltildi. Son genişletilmiş klavye testinde tanımsız F1 sabiti derleme hatası verdi; doğru keycode 59 ile düzeltildi. Başarısız ham kayıt saklandı ve düzeltilmiş test gerçek çalıştırmayla yeniden doğrulandı. Root ile yürütülen son kanıt döngüsünde unshare içinden test binary erişimi Permission denied verdi; zaten UID 0 olan host birim testinde gereksiz ikinci user namespace kaldırıldı, normal kullanıcı yolunda unshare korundu. Bu test başlatma hatasının kaydı da saklandı. Test silinmedi/atlanmadı.

R22 metadata testinin Size tıklamasının no-op kalmasını bekleyen satırı R23'te gerçek yeni işleve göre güncellendi. İki farklı gerçek dosya boyutu ile artan/azalan sıralama ve seçimin korunması sınanır; beklenti yalnızca PASS için gevşetilmedi.

Overview sunucusu: gerçek SO_PEERCRED, yanlış supervised PID reddi, 1000 istek, kayıt crash/restart durumları, boot durum eşlemesi, reserved/truncated/SCM_RIGHTS FD temizliği. Servis restart testi test kayıtlarını değiştirir; gerçek guest servis restart'ı değildir. Client: bozuk count/reserved/replay, timeout/disconnect, 10000 sessiz tick, rate limit, klavye ve render guard.

Gerçek host fixture üç freestanding uygulamayı gerçek compositor koduyla çalıştırır; legacy R20 istemci uyumluluğu, focus/input/configure ACK, minimize/maximize/restore, crash/reopen ve kaynak temizliği korunur. Sandbox-runtime grubunda surface peer mock kullanılır; bu grup gerçek compositor kanıtı diye sunulmaz.

14 seçili protokol/core/metadata ve init sandbox/seccomp/grant fonksiyonu byte/hash eşitliğiyle korunmuştur. Init dosyasının tamamı değiştiğinden bütün init değişmedi iddiası yoktur. Genel sandbox'a /proc, /sys veya yazma yetkisi eklenmedi.

## BUILD PASS

{len(native)} statik ELF: her biri 9 x86_64/ARM64 binary; machine kimliği, INTERP/DYNAMIC yokluğu ve SHA-256 doğrulandı. İki unsigned rootfs/initramfs yapısal build testi geçti. ARM64 ELF üretimi ARM64 runtime veya boot testi değildir. İmzalı GPT/ESP üretilmedi. Unsigned geliştirme UNPINNED varsayılanları üretim boot güveni değildir; kabul pipeline pin/ölçüm/policy değerlerini doğrulamalıdır.

## VISUAL REVIEW

{visual['r23_captures']} R23 native capture; 15 eşit-state R22/R23 karşılaştırması, ayrıca yeni Overview özellik karşılaştırması. Sonuncu farklı sayfaları gösterir ve eşit-state çift sayısına katılmaz. Host native C çıktısıdır, gerçek VirtualBox ekran görüntüsü değildir.

Adaptive duvar kağıdı/dock/ikon kimliği korundu. Görsel ilerleme Settings bilgi mimarisi, yeni Overview, Quick eylemleri ve Files sıralama/seçim geri bildirimindedir; tüm masaüstü baştan tasarlandı iddiası yoktur. 1280×800,1600×900,1920×1080 ve tema çıktıları üretilmiştir. Ayrıntılı görsel gözlem `R23_VISUAL_REVIEW_TR.md` içindedir. CPU/total RAM erişilemez; host PID 1 novad olmadığından servis/Secure Boot/TPM/verity host görünümünde Unavailable gösterilir. Local app/buffer/monotonic uptime gerçek veridir. Running pozitif registry PID'sidir; sağlık/ready sertifikası değildir.

## PERFORMANCE MEASURED

6 dengeli blok × 3 sürüm × 121 ölçülen frame; 20 warmup/run, tek host CPU affinity, aynı 1600×900 uygulama sahnesi; ham sıralı örnekler ve binary hash'leri tutuldu. Windows yükü/frekans/ısı kontrol edilemedi. Ölçüm CPU wall time'dır; ekran FPS'i veya input latency değildir.

| Sürüm | pooled p50 ms | pooled p95 ms | harness peak RSS KiB |
|---|---:|---:|---:|
{rows}

R22/R23 aynı sahneye göre kıyaslanır; Settings panel işlevleri farklıdır. Bileşen maliyetleri ve altı blok bootstrap belirsizliği JSON kanıtlarında bulunur. Küçük farklar kesin optimizasyon/kök neden kanıtı sayılmaz. Gölge/font/kompozisyon alt maliyetleri profile kaydındadır. Overview çizimi socket I/O yapmaz; otomatik telemetri en çok iki saniyede bir, manuel en çok saniyede bir. CPU/toplam RAM telemetrisi uygulanmadı.

R22→R23 pooled p50 yaklaşık %1,36 yüksek, p95 yaklaşık %0,18 düşük. Altı blok bootstrap ortalama fark aralığı p50 için [-%2,44,+%4,51], p95 için [-%5,90,+%3,95]; sıfır dahil. Kesin hızlanma veya anlamlı regresyon/kök neden iddiası yoktur. En yüksek R23 tek örnek 12,958 ms; aykırı örnekler silinmedi. Ayrı bileşen zamanları frame toplamına toplanmaz. Önceki tarihsel R22 p50 5,776/p95 6,429 farklı host ölçüm koşuluna aittir.

Bileşen ölçümü: `{json.dumps(cost,ensure_ascii=False)}`.

10.000 bounded gerçek host tema+metadata yenileme döngüsü sonucu: `{json.dumps(soak_brief,ensure_ascii=False)}`. Bu kısa host kararlılık testi; uzun süreli guest, 24 saat veya güç kesintisi testi değildir. Overview socket sunucusu ayrı 1000 istek negatif testinde sınanır; host soak güvenilir PID 1 Overview çalıştırmaz.

## Korunan kaynaklar

Başlangıç/bitiş manifests: `{json.dumps(integrity,ensure_ascii=False)}`.

R13/R14/R15 483 kayıt ve R16–R22 envanterleri fark göstermedi. R22 1901 kaynak/kanıt ve 18 native binary kaydı korundu. R25/R27 VM/disk/snapshot, firmware/TPM ve anahtarlar kullanılmadı/değiştirilmedi; bu VM disklerinin tamamı yeni hash'ten geçirildi iddiası yoktur. R13 imajı korunan manifestteki SHA-256 ile eşleşir.

## REAL GUEST PENDING / PRODUCTION READINESS PENDING

Guest boot, gerçek input/present, güvenlik boot zinciri ve uzun süreli çalışma yapılmadı. Kalıcı kullanıcı tercihi, CPU/total RAM, readiness health, tam HiDPI, GPU hızlandırma, 60 FPS ve günlük kullanım üretim kabulü PENDING.

`R23_ISOLATED_GUEST_ACCEPTANCE_TR.md` ayrı yeni disk/firmware/TPM/test trust chain planını tanımlar. Araçlar ve OVMF adayları salt okunur keşfedildi; exact kernel/firmware seçimi ve tüm boot girdisi kabulü henüz tamamlanmadı. Yeni anahtar, GPT/ESP, VM oluşturma/çalıştırma için ayrı açık onay gerekir. Bu görev o onay değildir; hiçbir aşama uygulanmadı.

R24 önceliği bu guest kabulünü tamamlamak, ardından kullanıcı oturumu/tercih broker'ı; gerçek PTY terminal ve ölçülebilir grafik altyapısıdır. Ayrıntı `R24_ENGINEERING_ROADMAP_TR.md`.

## Grupların gerçek sonuçları

{checks}

## Kanıt dizini

`evidence/r23/acceptance-results.json`, `native-binary-verification.json`, `security-source-identity.json`, `integrity-start.json`, `integrity-final.json`, `source.diff`, `native-code.diff`, `changed-files.json`, `production-source-sha256.json`, `visual-matrix.json`, `performance-study/`, `feature-cost.json`, `soak.json`, `guest-preflight.json`. Önceki başarısız test kaydı ve hedefli tekrar doğrulama kaydı da korunur.
"""
(r/'R23_TECHNICAL_REPORT_TR.md').write_text(text)
