"use strict";
const file=document.getElementById("file"),player=document.getElementById("player"),status=document.getElementById("status");let active=null;
file.addEventListener("change",()=>{player.pause();player.removeAttribute("src");player.load();if(active){URL.revokeObjectURL(active);active=null;}const chosen=file.files[0];if(!chosen){status.textContent="Henüz dosya seçilmedi.";return;}if(chosen.size>256*1024*1024){status.textContent="Bu sürümde dosya sınırı 256 MiB.";file.value="";return;}active=URL.createObjectURL(chosen);player.src=active;status.textContent=chosen.name+" · yükleniyor";});
player.addEventListener("loadedmetadata",()=>{status.textContent=(file.files[0]?.name||"Ses dosyası")+" · "+(Number.isFinite(player.duration)?Math.floor(player.duration)+" saniye":"süre bilinmiyor");});
player.addEventListener("error",()=>{status.textContent="Dosya açılamadı veya bu ses biçimi desteklenmiyor.";});
window.addEventListener("pagehide",()=>{player.pause();if(active){URL.revokeObjectURL(active);active=null;}});
