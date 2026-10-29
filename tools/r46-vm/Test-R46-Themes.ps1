$ErrorActionPreference='Stop'
$p=$PSScriptRoot;$id='afeecd0e-5b84-49d5-8cf0-f80e9b120ffa';$v=Join-Path $env:ProgramFiles 'Oracle\VirtualBox\VBoxManage.exe'
$b=New-Object -ComObject VirtualBox.VirtualBox;$m=$b.FindMachine($id);if($m.Name -ne 'NovaOS-R46-Desktop-Test' -or $m.SettingsFilePath -ne (Join-Path $p 'VMs\NovaOS-R46-Desktop-Test\NovaOS-R46-Desktop-Test.vbox')){throw 'Wrong own R46 VM'}
$s=Get-ChildItem -LiteralPath $p -Filter 'session-*.json'|Sort-Object LastWriteTime -Descending|Select-Object -First 1;$run=Get-Content -LiteralPath $s.FullName -Raw|ConvertFrom-Json;if($run.vm -ne $id -or $run.log -notmatch '^runtime-[0-9-]+\.log$'){throw 'Invalid session'};$log=Join-Path $p $run.log
function Serial { $f=[IO.File]::Open($log,'Open','Read','ReadWrite');try{$rd=[IO.StreamReader]::new($f);$rd.ReadToEnd()}finally{$f.Dispose()}}
function Keys([string[]]$k){& $v controlvm $id keyboardputscancode @k;if($LASTEXITCODE -ne 0){throw 'Key input failed'}}
function Click([int]$x,[int]$y){& (Join-Path $p 'Mouse-Probe-R46.ps1') -X $x -Y $y -Action Click}
function Shot([string]$n){& $v controlvm $id screenshotpng (Join-Path $p ('REAL-VIRTUALBOX\ready-'+$n+'.REAL-VM.png'));if($LASTEXITCODE -ne 0){throw 'Screenshot failed'}}
$end=[DateTime]::UtcNow.AddSeconds(90);while(!(Test-Path -LiteralPath $log) -or !(Serial).Contains('desktop_task_model=COMMITTED')){if([DateTime]::UtcNow -gt $end){throw 'Desktop not ready'};Start-Sleep -Milliseconds 200};Start-Sleep -Seconds 2
Shot 'desktop';Add-Type -AssemblyName System.Drawing;$im=[Drawing.Image]::FromFile((Join-Path $p 'REAL-VIRTUALBOX\ready-desktop.REAL-VM.png'));try{if($im.Width -ne 1280 -or $im.Height -ne 800){throw 'Geometry mismatch'}}finally{$im.Dispose()}
function Theme([int]$choice,[int]$x){$offset=(Serial).Length;Keys @('1d','38','10','90','b8','9d');Start-Sleep -Milliseconds 500;Click $x 160;$end=[DateTime]::UtcNow.AddSeconds(15);while(!(Serial).Substring($offset).Contains('session_theme='+$choice+' generation=')){if([DateTime]::UtcNow -gt $end){throw 'Theme change not observed'};Start-Sleep -Milliseconds 100};Keys @('01','81');Start-Sleep -Milliseconds 500}
Theme 1 1096;Click 500 760;Start-Sleep -Milliseconds 500;Click 520 412;Start-Sleep -Milliseconds 600;Shot 'graphite-calendar';Keys @('01','81')
Theme 0 994;Click 500 760;Start-Sleep -Milliseconds 500;Click 520 468;Start-Sleep -Milliseconds 600;Shot 'light-clock';Keys @('01','81')
Theme 2 1196;Click 500 760;Start-Sleep -Milliseconds 500;Shot 'applications';Click 520 468;Start-Sleep -Seconds 1;Shot 'adaptive-clock'
@{status='COMMANDS_PASS_VISUAL_REVIEW_PENDING';themes=@('Graphite','Light','Adaptive');vm=$id;guest_left_running=$true;serial=$run.log}|ConvertTo-Json|Set-Content -Encoding utf8 -LiteralPath (Join-Path $p 'themes-results.json')
