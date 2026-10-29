$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($PSScriptRoot);. (Join-Path $root 'Vm-Helpers.ps1');$exe=Join-Path $env:ProgramFiles 'Oracle\VirtualBox\VBoxManage.exe';$box=New-Object -ComObject VirtualBox.VirtualBox;$vm=$box.FindMachine('afeecd0e-5b84-49d5-8cf0-f80e9b120ffa')
$expected=Join-Path $root 'VMs\NovaOS-R46-Desktop-Test\NovaOS-R46-Desktop-Test.vbox'
if(![string]::Equals([IO.Path]::GetFullPath($vm.SettingsFilePath),$expected,[StringComparison]::OrdinalIgnoreCase)){throw 'Wrong portable VM folder; no shutdown sent.'}
$state=Get-Week4State $exe $vm.Id
if($state -eq 'poweroff'){Write-Host 'VM is already powered off.';exit 0}
if($state -ne 'running'){throw 'VM is not running normally; no forced state change attempted.'}
$index=Get-ChildItem -LiteralPath $root -Filter 'session-*.json'|Sort-Object LastWriteTime -Descending|Select-Object -First 1
if(!$index){throw 'No console session index. No forced shutdown attempted.'}
$s=Get-Content -LiteralPath $index.FullName -Raw|ConvertFrom-Json
if($s.vm -ne $vm.Id -or $s.queue -notmatch '^commands-[0-9-]+\.jsonl$'){throw 'Invalid console session index'}
@{command='poweroff'}|ConvertTo-Json -Compress|Add-Content -Encoding utf8 (Join-Path $root $s.queue)
for($i=0;$i -lt 20;$i++){Start-Sleep -Seconds 1;if((Get-Week4State $exe $vm.Id) -eq 'poweroff'){Write-Host 'NovaOS completed normal shutdown.';exit 0}}
throw 'Normal shutdown was not confirmed in 20 seconds. Inspect serial log; no reset or force poweroff was used.'
