param([switch]$Reboot)
$ErrorActionPreference='Stop'
$baseRoot=$PSScriptRoot;$id='fd0c1323-3f39-462a-9473-001b085f0043';$v=Join-Path $env:ProgramFiles 'Oracle\VirtualBox\VBoxManage.exe'
$box=New-Object -ComObject VirtualBox.VirtualBox;$machine=$box.FindMachine($id)
$cfg=Join-Path $baseRoot 'VMs\NovaOS-R44-PDF-Test\NovaOS-R44-PDF-Test.vbox'
if($machine.Name -ne 'NovaOS-R44-PDF-Test' -or ![string]::Equals($machine.SettingsFilePath,$cfg,[StringComparison]::OrdinalIgnoreCase) -or !((& $v showvminfo $id --machinereadable) -contains 'VMState="running"')){throw 'R44 running VM identity/path guard failed'}
$sessionFile=Get-ChildItem -LiteralPath $baseRoot -Filter 'session-*.json'|Sort-Object LastWriteTime -Descending|Select-Object -First 1
$run=Get-Content -LiteralPath $sessionFile.FullName -Raw|ConvertFrom-Json
if($run.vm -ne $id -or $run.queue -notmatch '^commands-[0-9-]+\.jsonl$' -or $run.log -notmatch '^runtime-[0-9-]+\.log$'){throw 'Invalid R44 session'}
$logPath=Join-Path $baseRoot $run.log;$queuePath=Join-Path $baseRoot $run.queue;$images=Join-Path $baseRoot 'REAL-VIRTUALBOX';New-Item -ItemType Directory -Force -Path $images|Out-Null
$tag=if($Reboot){'reboot'}else{'first'}
function Read-Serial {
 if(!(Test-Path -LiteralPath $logPath)){return ''}
 $fs=[IO.File]::Open($logPath,'Open','Read','ReadWrite');try{$reader=[IO.StreamReader]::new($fs);return $reader.ReadToEnd()}finally{$fs.Dispose()}
}
function Root-Read([string]$command,[string]$expectedText){
 $offset=(Read-Serial).Length;@{command=$command}|ConvertTo-Json -Compress|Add-Content -Encoding utf8 -LiteralPath $queuePath
 $deadline=[DateTime]::UtcNow.AddSeconds(30)
 do{$all=Read-Serial;$part=$all.Substring([Math]::Min($offset,$all.Length));if($part.Contains($expectedText)){return};Start-Sleep -Milliseconds 100}while([DateTime]::UtcNow -lt $deadline)
 throw ('Guest command result not observed: '+$command)
}
function Keys([string[]]$scan){& $v controlvm $id keyboardputscancode @scan;if($LASTEXITCODE -ne 0){throw 'Keyboard failed'}}
function Type-Line([string]$line){& $v controlvm $id keyboardputstring $line;if($LASTEXITCODE -ne 0){throw 'Typing failed'};Keys @('1c','9c');Start-Sleep -Milliseconds 700}
function Click([int]$x,[int]$y){& (Join-Path $baseRoot 'Mouse-Probe-R44.ps1') -X $x -Y $y -Action Click}
function Shot([string]$name){& $v controlvm $id screenshotpng (Join-Path $images ($tag+'-'+$name+'.REAL-VM.png'));if($LASTEXITCODE -ne 0){throw 'Screenshot failed'}}
function Ctrl([string]$code){Keys @('1d',$code,([Convert]::ToString(([Convert]::ToInt32($code,16)+128),16)),'9d');Start-Sleep -Milliseconds 500}
function Text-Only([string]$s){& $v controlvm $id keyboardputstring $s;if($LASTEXITCODE -ne 0){throw 'Typing failed'};Start-Sleep -Milliseconds 500}
function Raise-Terminal {Keys @('1d','38','14','94','b8','9d');Start-Sleep -Milliseconds 600}

$tag='extra'
# Focus the exposed Terminal title; Ctrl+Alt+T is a toggle, not an unconditional raise.
Click 600 85
Type-Line 'sha256sum Lecture.pdf > pdf-verified.hash'
Root-Read 'cat /run/nova/user-session/home/pdf-verified.hash' '33188b82ad0335f9bc2eb092d75eac5ad179d4ea15fbf801e1fc62c79f1003c6'
Type-Line 'nova-pdf Lecture.pdf; echo INTERRUPT_DONE > pdf-interrupt.log';Start-Sleep -Seconds 2;Shot 'pdf-before-interrupt';Ctrl '2e'
Root-Read 'cat /run/nova/user-session/home/pdf-interrupt.log' 'INTERRUPT_DONE'
Type-Line 'stty -g > pdf-mode-int.txt; cmp -s pdf-mode-before.txt pdf-mode-int.txt; echo MODE_INT_$? > pdf-mode-int.log'
Root-Read 'cat /run/nova/user-session/home/pdf-mode-int.log' 'MODE_INT_0'
Keys @('1d','38','10','90','b8','9d');Start-Sleep -Milliseconds 500
foreach($theme in @(@{x=1416;id=1;name='graphite'},@{x=1314;id=0;name='light'},@{x=1516;id=2;name='adaptive'})){
 $offset=(Read-Serial).Length;Click $theme.x 160;Start-Sleep -Seconds 1
 $now=Read-Serial;$new=$now.Substring([Math]::Min($offset,$now.Length));if(!$new.Contains('session_theme='+$theme.id+' generation=')){throw ('Real theme event missing: '+$theme.name)}
 Shot ('theme-'+$theme.name)
}
Keys @('01','81');Root-Read 'cat /run/nova/performance.state' 'R24_PERF '
@{status='COMMANDS_PASS_VISUAL_REVIEW_PENDING';pdf_fixture_sha256='33188b82ad0335f9bc2eb092d75eac5ad179d4ea15fbf801e1fc62c79f1003c6';interrupt='actual Ctrl+C returned to shell';termios='MODE_INT_0';themes='real session_theme events1,0,2 and captures';scope='actual R44 VirtualBox PS2; physical manual/soak pending'}|ConvertTo-Json|Set-Content -Encoding utf8 (Join-Path $baseRoot 'extra-guest-results.json')
