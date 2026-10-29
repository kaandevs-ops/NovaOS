# NovaOS R24 — gerçek VirtualBox denemesi

Makine: **NovaOS-R24-Test**. R23-Test veya korunan R25/R27 makinelerini açmanız/değiştirmeniz gerekmiyor. R24 şu anda açık bırakıldı. Sonradan kapatırsanız bu klasördeki `R24_AC.cmd` sadece mevcut R24 kaydını başlatır; disk/firmware/TPM ayarlarını değiştirmez.

1. Fareyi masaüstünde ve dock üzerinde hareket ettirin.
2. Files dock simgesine basın. Başlıktan sürükleyin, sağ alt köşeden boyutlandırın.
3. Documents içinde kaydırmayı deneyin; test klasörü 65 gerçek dosya içerir, mevcut motor listeyi 48 öğede sınırlar. Bu sınır kaldırılmadı.
4. Bir dosyayı seçip **I** tuşuyla gerçek dosya bilgilerini açın, **Esc** ile kapatın.
5. **Alt+Tab** uygulama değiştirir; **Alt+M** küçültür; dock veya uygulama menüsü geri getirir. **Alt+F10** büyütür/geri yükler, **Alt+F4** kapatır. Files tekrar açılabilir.
6. Sağ üst sistem kontrolü veya **Ctrl+Alt+Q** Quick Settings. **Ctrl+Alt+S** Appearance. Ayarlar açıkken **F2** Graphics diagnostics; **F1** Appearance/Overview. **Ctrl+Alt+I** System Overview.
7. Light/Graphite/Adaptive temaları gerçek uygulamalara uygulanır. Tercihler oturum bazlıdır.

Mouse yakalamasından çıkmak için VirtualBox'ın Host tuşunu kullanın (genellikle Sağ Ctrl; pencerenin alt çubuğunda yazar).

Bu bir geliştirme adayıdır. Gerçek VM açılışı ve otomatik etkileşimler sınandı; rahat kullanımın son kabulü sizin değerlendirmenizi bekliyor. GPU hızlandırma, garantili 60 FPS, tam HiDPI, günlük kullanım/üretim kabulü, terminal ve ağ desteği ilan edilmiyor. Kendi önemli dosyalarınızı buraya taşımayın; örnekler sadece yeni R24 Data bölümündeki test verileridir.

Kapatmak için `R24_KAPAT.cmd`, güvenilir seri console üzerinden PID 1'e `poweroff` gönderir. Düzenli kapanış isteğidir; zorla kapatma yapmaz. Eski VM'leri zorla kapatmayın veya snapshot silmeyin.
