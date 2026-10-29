<p align="center"><img src="docs/media/banner.svg" alt="NovaOS: Linux temeli üzerinde özgün native masaüstü" width="100%"></p>

<p align="center"><strong>Linux çekirdeği üzerinde özgün masaüstü ve kullanıcı alanı projesi.</strong><br>Native C compositor · kendi shell ve servis yönetimi · sınırlandırılmış uygulama entegrasyonu</p>

<p align="center"><a href="README.md">English</a> · <a href="docs/ARCHITECTURE.md">Mimari</a> · <a href="docs/BUILDING.md">Derleme</a> · <a href="docs/ROADMAP.md">Yol haritası</a></p>

> **Bu depo temizlenmiş bir geliştirme kaynak yayınıdır.** Açılabilir VM paketi veya üretime hazır işletim sistemi değildir. Üstteki banner özgün kimlik çizimidir; ekran görüntüsü değildir. Linux çekirdeği ve sisteme entegre edilen üçüncü taraf yazılımlar NovaOS tarafından sıfırdan yazılmadı.

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
