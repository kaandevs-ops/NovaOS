param([switch]$Reboot)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'Test-R46-Guest.ps1') -Reboot:$Reboot
if($actualWidth -ne 1280 -or $actualHeight -ne 800){throw 'R46 desktop input coordinates require validated 1280x800; no blind input'}
if($Reboot){Root-Read 'cat /run/nova/user-session/home/R46-Power-Proof.txt' 'R46_DESKTOP_POWER_PERSIST'}
Click $terminalX 20;Type-Line 'echo R46_DESKTOP_POWER_PERSIST > R46-Power-Proof.txt; nova-sync R46-Power-Proof.txt > flush.log; echo SYNC_EXIT_$? >> flush.log'
Root-Read 'cat /run/nova/user-session/home/flush.log' 'SYNC_EXIT_0'
Root-Read 'cat /run/nova/user-session/home/R46-Power-Proof.txt' 'R46_DESKTOP_POWER_PERSIST'
Click 500 760;Start-Sleep -Milliseconds 600;Shot 'applications-clock-calendar'
Click 520 412;Start-Sleep -Milliseconds 700;Shot 'calendar-initial'
Keys @('e0','4d','e0','cd');Start-Sleep -Milliseconds 500;Shot 'calendar-next'
Keys @('e0','4b','e0','cb');Start-Sleep -Milliseconds 400;Shot 'calendar-previous'
Keys @('01','81');Click 500 760;Start-Sleep -Milliseconds 500;Click 520 468;Start-Sleep -Milliseconds 700;Shot 'clock-first';Start-Sleep -Seconds 2;Shot 'clock-ticking';Keys @('01','81')
Keys @('1d','38','10','90','b8','9d');Start-Sleep -Milliseconds 600;Click 1200 290;Start-Sleep -Milliseconds 600;Shot 'power-confirm-default-cancel';Keys @('1c','9c');Start-Sleep -Milliseconds 700
if(!((& $v showvminfo $id --machinereadable) -contains 'VMState="running"')){throw 'Default Cancel must keep guest running'}
Root-Read 'services' 'compositor'
Keys @('1d','38','10','90','b8','9d');Start-Sleep -Milliseconds 600;Click 1200 290;Start-Sleep -Milliseconds 500;Keys @('e0','4d','e0','cd');Shot 'power-confirm-selected';Keys @('1c','9c')
$deadline=[DateTime]::UtcNow.AddSeconds(45)
do{$info=& $v showvminfo $id --machinereadable;if($info -contains 'VMState="poweroff"'){break};Start-Sleep -Milliseconds 300}while([DateTime]::UtcNow -lt $deadline)
if(!($info -contains 'VMState="poweroff"')){throw 'Desktop poweroff not confirmed; VM retained, no force shutdown'}
Start-Sleep -Seconds 1;$serial=Read-Serial
# VBox UART tail may omit a userspace line. Require kernel powerdown + actual off; independent offline FS proof is recorded separately.
foreach($needle in @('authenticated compositor requested ordered poweroff','reboot: Power down')){if(!$serial.Contains($needle)){throw ('Shutdown evidence missing: '+$needle)}}
@{status='COMMANDS_PASS_VISUAL_REVIEW_PENDING';vm=$id;reboot=[bool]$Reboot;scope='Real isolated VirtualBox 1280x800; launcher/calendar/clock screenshots, default Cancel, authenticated GUI poweroff';normal_desktop_poweroff='PASS';forced_shutdown=$false;serial=$run.log}|ConvertTo-Json|Set-Content -Encoding utf8 -LiteralPath (Join-Path $baseRoot ($tag+'-desktop-results.json'))
Write-Host 'R46_DESKTOP_NORMAL_POWEROFF_PASS; images require visual review'
