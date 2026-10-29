$ErrorActionPreference='Stop'
$vmId='afeecd0e-5b84-49d5-8cf0-f80e9b120ffa'
$root=[IO.Path]::GetFullPath($PSScriptRoot)
. (Join-Path $root 'Vm-Helpers.ps1')
$cfg=Join-Path $root 'VMs\NovaOS-R46-Desktop-Test\NovaOS-R46-Desktop-Test.vbox'
$exe=Join-Path $env:ProgramFiles 'Oracle\VirtualBox\VBoxManage.exe'
if(!(Test-Path -LiteralPath $exe)){throw 'Install Oracle VirtualBox 7.2.20 first. WSL is not needed to run this VM.'}
$version=(& $exe --version | Out-String).Trim()
if(!$version.StartsWith('7.2.20')){throw "Validated version is VirtualBox 7.2.20; found $version. Rehearse compatibility separately."}
if(!(Test-Path -LiteralPath $cfg)){throw 'Extract the complete portable ZIP to a NEW folder first.'}
[xml]$xml=[IO.File]::ReadAllText($cfg)
$folder=[IO.Path]::GetDirectoryName($cfg)
foreach($disk in $xml.SelectNodes("//*[local-name()='HardDisk']")){
 if([IO.Path]::IsPathRooted($disk.location)){throw 'External absolute disk path is not accepted.'}
 $path=[IO.Path]::GetFullPath((Join-Path $folder $disk.location))
 if(!$path.StartsWith($folder+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or !(Test-Path -LiteralPath $path)){throw 'Missing or external VM disk.'}
}
if(!(Test-Path -LiteralPath (Join-Path $folder 'NovaOS-R46-Desktop-Test.nvram'))){throw 'UEFI/TPM test state is missing. Do not reset or disable Secure Boot.'}
$box=New-Object -ComObject VirtualBox.VirtualBox
$vm=$null
try{$vm=$box.FindMachine($vmId)}catch{}
if($null -eq $vm){& $exe registervm $cfg;if($LASTEXITCODE -ne 0){throw 'VM registration failed'};$vm=$box.FindMachine($vmId)}
if(![string]::Equals([IO.Path]::GetFullPath($vm.SettingsFilePath),$cfg,[StringComparison]::OrdinalIgnoreCase)){throw 'This VM UUID is already registered from another folder. Use that existing copy or VirtualBox Clone; no files were overwritten.'}
$state=Get-Week4State $exe $vmId
if($state -eq 'running'){Write-Host 'NovaOS-R46-Desktop-Test is already running.';exit 0}
if($state -ne 'poweroff'){throw 'VM is not powered off. Finish a normal shutdown first; this launcher never forces power off.'}
$unlockDeadline=[DateTime]::UtcNow.AddSeconds(15)
while($vm.SessionState -ne 1){
 if([DateTime]::UtcNow -ge $unlockDeadline){throw 'VirtualBox session is still unlocking; retry normal start later. No force action was taken.'}
 Start-Sleep -Milliseconds 250
}
$stamp=Get-Date -Format 'yyyyMMdd-HHmmssfff';$log="runtime-$stamp.log";$queue="commands-$stamp.jsonl"
@{vm=$vmId;log=$log;queue=$queue;created=$stamp}|ConvertTo-Json|Set-Content -Encoding utf8 (Join-Path $root "session-$stamp.json")
$bridge=Join-Path $root 'Console-Bridge.ps1'
Start-Process powershell.exe -WindowStyle Hidden -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$bridge+'"'),'-LogName',$log,'-QueueName',$queue)
& $exe startvm $vmId --type gui
if($LASTEXITCODE -ne 0){throw 'VM start failed; inspect VirtualBox logs. No force/reset was attempted.'}
Write-Host 'Wait for the NovaOS desktop. Ctrl+Alt+T opens Terminal; run: nova-guide'
Write-Host "Serial evidence: $log"
