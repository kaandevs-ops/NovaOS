<p align="center"><img src="docs/media/banner.png" alt="Kaan Tekin — AI, Robotics and Autonomous Systems" width="100%"></p>

<h1 align="center">NovaOS</h1>

<p align="center"><strong>Linux çekirdeği üzerinde özgün masaüstü ve kullanıcı alanı projesi.</strong><br>Native C compositor · kendi shell ve servis yönetimi · sınırlandırılmış uygulama entegrasyonu</p>

<p align="center"><a href="README.md">English</a> · <a href="docs/ARCHITECTURE.md">Mimari</a> · <a href="docs/BUILDING.md">Derleme</a> · <a href="docs/ROADMAP.md">Yol haritası</a></p>

> **Bu depo temizlenmiş bir geliştirme kaynak yayınıdır.** Açılabilir VM paketi veya üretime hazır işletim sistemi değildir. Banner profilimde ve Unity repomda kullandığım mevcut dosyadır. Linux ve üçüncü taraf yazılımlar NovaOS tarafından sıfırdan yazılmadı.

## Görseller ve video

Bunlar bu deponun **gerçek native C CPU renderer'ından**, Linux host üzerinde 1600 × 900 çözünürlükte üretilmiştir. Mevcut özgün Graphite wave arka planı kullanılır. VirtualBox görüntüsü veya HTML maketi değildir. Bu host galerisinde bağlı uygulama süreci yoktur; Software ekranı uydurma kurulum sonucu yerine bekleme durumunu gösterir.

[![Native renderer: yedi kareden oluşan dizi](docs/media/native-renderer-preview.HOST-NOT-VM.gif)](docs/media/native-renderer-walkthrough.HOST-NOT-VM.mp4)

**[21 saniyelik renderer videosunu izle/indir](docs/media/native-renderer-walkthrough.HOST-NOT-VM.mp4)** · [Hareketli önizleme](docs/media/native-renderer-preview.HOST-NOT-VM.gif) · [Üretim yöntemi ve hash'ler](docs/MEDIA.md)

Video, okunabilirlik için bekletilen yedi gerçek renderer karesinden oluşan bir dizidir. Kesintisiz kullanıcı kaydı, FPS ölçümü veya guest performans kanıtı değildir.

| Masaüstü | Uygulama menüsü |
| --- | --- |
| ![Masaüstü: native host çıktısı](docs/media/01-desktop.HOST-NOT-VM.png) | ![Menü: native host çıktısı](docs/media/02-applications.HOST-NOT-VM.png) |

| Calm Light | Graphite Pro |
| --- | --- |
| ![Açık tema: native host çıktısı](docs/media/04-calm-light.HOST-NOT-VM.png) | ![Koyu tema: native host çıktısı](docs/media/05-graphite.HOST-NOT-VM.png) |

| Quick Settings | Software: bağlı uygulama süreci yok |
| --- | --- |
| ![Hızlı ayarlar: native host çıktısı](docs/media/06-quick-settings.HOST-NOT-VM.png) | ![Software bekleme durumu: native host çıktısı](docs/media/07-software-no-peer.HOST-NOT-VM.png) |

## Ayrıntılı özellik rehberi

| Yetenek | Kaynaklarda uygulanan kapsam |
| --- | --- |
| **Pencereler ve giriş** | Odak/katman, taşıma/boyutlandırma, küçültme/büyütme/geri getirme, kapat/yeniden aç ve desteklenen klavye geçişleri |
| **Masaüstü** | Klasör açma, sınırlı metin önizleme, sağ tık menüsü, seçim, kontrollü çöp kutusu/geri yükleme ve çakışma hataları |
| **Nova Files** | Salt okunur dizin yetkisi, geri/ileri geçmişi, breadcrumb, filtre, ad/tür/boyut sıralaması ve mevcut metadata |
| **Terminal** | Gerçek oturum/alt süreçler, çalışma dizini/argümanlar, Türkçe-Q Shift/Caps/AltGr ve tırnaklanmış yol ekleme |
| **Görünüm** | Calm Light, Graphite Pro, Adaptive; ortak UI; Settings/Quick Settings ve oturum tema olayları |
| **Araçlar** | Takvim, saat ve onaylı normal kapatma arayüzleri |
| **Yerel uygulamalar** | SQLite, Lua, dynamic Lua, terminal 2048 kataloğu; kur/aç/durum/kaldır ve kurulu uygulama dock entegrasyonu |
| **İnternet paketleri** | Depo istemcisi, politika/doğrulama, sınırlı aktarım ve yayın araçları; genel public mağaza adresi henüz yok |
| **Tarayıcı / belgeler** | Harici Linux tarayıcısı/X uyumluluğu, belge/PDF ve izole MuPDF worker kaynakları; runtime binary'leri dahil değil |
| **Sistem** | `novad`, manifest/principal, uygulama sandbox/limitleri, memfd/SCM_RIGHTS/surface denetimi, ağ ve depolama/kurtarma |
| **Tanılama** | Overview/Graphics veri yolları, CPU çizim ölçümleri ve açık unavailable durumları; guest FPS çıkarımı yapılmaz |

Pencere/masaüstü işlemleri, Files, terminal, tema/ayarlar, saat/takvim, uygulama ve paket yönetimi, tarayıcı/PDF, ağ, depolama ve güvenlik: [özellikler, kaynak yolları, kısayollar ve kabul sınırları](docs/FEATURES.md). Kaynağın bulunması yeni public guest kabulünün geçtiği anlamına gelmez.

## Açılabilir VM indirilebilir mi?

Eski temiz VM'nin özel ortamda açılış kanıtları var; fakat başlatıcı ve binary kaynak/lisans dağıtımı eksikleri ayrıdır. VM ZIP'i **henüz Release olarak yayımlanmıyor**. [Somut kalan işler](docs/VM_RELEASE.md). GitHub'ın otomatik kaynak ZIP'i kurulabilir işletim sistemi değildir.

## NovaOS nedir?

NovaOS; Linux kernel üzerine kendi rootfs/initramfs düzenini, `novad` PID 1/servis entegrasyonunu, native C compositor ve masaüstü kabuğunu, dosya yöneticisini, terminal ve uygulama altyapısını geliştiren bir projedir. Hazır Ubuntu masaüstünün tema değiştirilmiş hali değildir.

Hedef; rafine ve hızlı bir masaüstü, gerçek uygulamalar, genişletilebilirlik ve dar yetkilerdir. Her Linux uygulamasının veya oyunun şimdiden çalıştığı iddia edilmez.

## Kaynaklarda neler var?

| Alan | İçerik |
| --- | --- |
| Masaüstü | Compositor, shell, dock, odak/pencere yönetimi ve ortak UI |
| Files | Dizin gezinmesi, filtreleme/sıralama ve guest adaptörleri |
| Terminal | Gerçek süreç/oturum altyapısı ve Türkçe klavye desteği |
| Uygulamalar | Katalog, manifest, paket yaşam döngüsü ve tarayıcı entegrasyonu |
| Sistem | Principal, FD aktarımı, sandbox, servis ve boot kaynakları |

## Kaynak kontrollerini çalıştırma

Linux/WSL üzerinde Bash, Python 3, GCC, Clang ve lld gerekir:

```sh
git clone https://github.com/kaandevs-ops/NovaOS.git
cd NovaOS
python3 scripts/audit-public.py
bash scripts/check-public.sh
```

Bu komutlar seçili bileşenleri kontrol eder; tam işletim sistemi imajı oluşturmaz ve VM açmaz. [Kurulum ve kapsam](docs/BUILDING.md), [test sonuçları](docs/VALIDATION.md).

## Temizlik ve kabul sınırları

Ders/sunum dosyaları, öğrenci belgeleri, Windows uzaktan bağlantı araçları, kişisel kayıtlar, özel anahtarlar, kullanıcı verileri, VM diskleri ve firmware/TPM durumu yayımlanmaz. Yalnız renderer için üretilmiş iki sayfalık test PDF'si korunur. Kökeni doğrulanamayan eski fotoğraf/ikon rasteri yerine nötr kaynak varlıkları kullanılır.

Bu değiştirilmiş kaynak kopyasının tam rootfs derlemesi, imzalı guest açılışı, gerçek donanım, tam Unicode, GPU ve üretim kabulü bekliyor. Önceki özel VM testleri bu kopyaya otomatik PASS sayılmaz. Genel internet mağazası ve sınırsız Ubuntu paket uyumu yoktur.

## Lisans

Özgün NovaOS kodu ve belgeleri, diğer projelerimdeki gibi **Apache-2.0** kullanır. Dosya bazındaki ve üçüncü taraf lisansları korunur; MuPDF'ye bağlı worker **AGPL-3.0-or-later** istisnasıdır. [NOTICE](NOTICE), [lisans kapsamı](docs/LICENSING.md), [üçüncü taraf bildirimleri](THIRD_PARTY_NOTICES.md).

[Katkı rehberi](CONTRIBUTING.md) · [Güvenlik](SECURITY.md) · [Yol haritası](docs/ROADMAP.md)

Geliştiren: [Kaan Tekin / kaandevs-ops](https://github.com/kaandevs-ops).
