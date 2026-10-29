$ErrorActionPreference='Stop'
$vbox='C:\Program Files\Oracle\VirtualBox\VBoxManage.exe'
$id='8e2cd845-2247-4b09-9bfd-0fbebcce4d23'
$info=&$vbox showvminfo $id --machinereadable
if($LASTEXITCODE -ne 0){throw 'Bu bilgisayarda doğrulanmış R24 VM kaydı bulunamadı.'}
if($info -contains 'VMState="poweroff"'){
 $tag='manual-'+(Get-Date -Format 'yyyyMMdd-HHmmss')
 $queue=$tag+'-commands.jsonl';$log=$tag+'-serial.log'
 Start-Process powershell.exe -WindowStyle Hidden -ArgumentList '-NoProfile','-ExecutionPolicy','Bypass','-File',($PSScriptRoot+'\Console-Bridge.ps1'),'-LogName',$log,'-QueueName',$queue -RedirectStandardError ($PSScriptRoot+'\'+$tag+'-bridge-error.log')
 @{vm_uuid=$id;queue=$queue;log=$log}|ConvertTo-Json|Set-Content ($PSScriptRoot+'\console-current.json')
 &$vbox startvm $id --type gui;if($LASTEXITCODE -ne 0){throw 'R24 başlatılamadı.'}}
elseif($info -contains 'VMState="running"'){Write-Host 'NovaOS-R24-Test zaten açık. VirtualBox penceresine geçebilirsiniz.'}
else{Write-Host 'VM açık/kapalı dışında bir durumda; otomatik değişiklik yapılmadı.'}
