param([int]$X,[int]$Y,[ValidateSet('Move','Click','Drag')][string]$Action='Move',[int]$DX=0,[int]$DY=0)
$ErrorActionPreference='Stop'
$box=New-Object -ComObject VirtualBox.VirtualBox
$session=New-Object -ComObject VirtualBox.Session
$vm=$box.FindMachine('fd0c1323-3f39-462a-9473-001b085f0043')
if($vm.Name -ne 'NovaOS-R44-PDF-Test'){throw 'Wrong VM'}
$vm.LockMachine($session,1)
try {
 $mouse=$session.Console.Mouse
 $mouse.PutMouseEvent(0,0,0,0,0)
 for($i=0;$i -lt 24;$i++){$mouse.PutMouseEvent(-100,-100,0,0,0);Start-Sleep -Milliseconds 8}
 $left=$X;$top=$Y
 while($left -gt 0 -or $top -gt 0){$mx=[Math]::Min(100,$left);$my=[Math]::Min(100,$top);$mouse.PutMouseEvent($mx,$my,0,0,0);$left-=$mx;$top-=$my;Start-Sleep -Milliseconds 15}
 Start-Sleep -Milliseconds 250
 if($Action -eq 'Click'){$mouse.PutMouseEvent(0,0,0,0,1);Start-Sleep -Milliseconds 70;$mouse.PutMouseEvent(0,0,0,0,0)}
 if($Action -eq 'Drag'){$mouse.PutMouseEvent(0,0,0,0,1);Start-Sleep -Milliseconds 70;for($i=0;$i -lt 10;$i++){$mouse.PutMouseEvent([int]($DX/10),[int]($DY/10),0,0,1);Start-Sleep -Milliseconds 25};$mouse.PutMouseEvent(0,0,0,0,0)}
 Start-Sleep -Milliseconds 250
 Write-Output "$Action at $X,$Y delta $DX,$DY submitted to R44 virtual PS/2 input"
} finally {$session.UnlockMachine()}
