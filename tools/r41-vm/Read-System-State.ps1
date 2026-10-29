param([ValidateSet('All','Boot','Services','Mounts')][string]$View='All')
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$file=Get-ChildItem -LiteralPath $root -Filter 'session-*.json'|Sort-Object LastWriteTime -Descending|Select-Object -First 1
if(!$file){throw 'Start the portable VM with Start-Week4.ps1 first.'}
$s=Get-Content -LiteralPath $file.FullName -Raw|ConvertFrom-Json
if($s.vm -ne 'add8ba49-841e-4a17-981d-efcb3167cea6' -or $s.queue -notmatch '^commands-[0-9-]+\.jsonl$' -or $s.log -notmatch '^runtime-[0-9-]+\.log$'){throw 'Invalid session evidence index'}
$commands=if($View -eq 'Boot'){@('boot','firmware','tpm','verity')}elseif($View -eq 'Services'){@('services')}elseif($View -eq 'Mounts'){@('layout','mounts')}else{@('boot','services','layout','mounts','firmware','tpm','verity')}
foreach($c in @('')+$commands){@{command=$c}|ConvertTo-Json -Compress|Add-Content -Encoding utf8 (Join-Path $root $s.queue);Start-Sleep -Seconds 1}
Get-Content -LiteralPath (Join-Path $root $s.log) -Tail 160
