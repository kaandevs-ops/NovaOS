$ErrorActionPreference='Stop'
$c=Get-Content ($PSScriptRoot+'\console-current.json') -Raw|ConvertFrom-Json
if($c.vm_uuid -ne '8e2cd845-2247-4b09-9bfd-0fbebcce4d23'){throw 'R24 console kimliği geçersiz.'}
if($c.queue -notmatch '^[a-zA-Z0-9-]+-commands\.jsonl$'){throw 'Console kuyruğu geçersiz.'}
[IO.File]::AppendAllText(($PSScriptRoot+'\'+$c.queue),"{`"command`":`"poweroff`"}`n",[Text.UTF8Encoding]::new($false))
Write-Host 'Yalnızca R24 PID 1 console kuyruğuna düzenli poweroff isteği eklendi. VM yanıt vermiyorsa zorla kapatma yapılmaz.'
