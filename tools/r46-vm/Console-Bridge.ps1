param([string]$LogName='baseline-serial.log',[string]$QueueName='baseline-commands.jsonl',[switch]$PollPerformance)
$ErrorActionPreference='Stop'
$base=$PSScriptRoot
$pipe=[System.IO.Pipes.NamedPipeClientStream]::new('.','NovaOS-R46-Console',[System.IO.Pipes.PipeDirection]::InOut,[System.IO.Pipes.PipeOptions]::Asynchronous)
$pipe.Connect(60000)
$log=[System.IO.FileStream]::new((Join-Path $base $LogName),[System.IO.FileMode]::CreateNew,[System.IO.FileAccess]::Write,[System.IO.FileShare]::ReadWrite)
$bytes=[byte[]]::new(8192);$read=$pipe.ReadAsync($bytes,0,$bytes.Length);$sent=0;$perfLast=[DateTime]::UtcNow
try { while($pipe.IsConnected) {
 if($read.IsCompleted){$count=$read.GetAwaiter().GetResult();if($count -eq 0){break};$log.Write($bytes,0,$count);$log.Flush();$read=$pipe.ReadAsync($bytes,0,$bytes.Length)}
 $queue=Join-Path $base $QueueName
 if(Test-Path -LiteralPath $queue){try{$qf=[IO.FileStream]::new($queue,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite);$qr=[IO.StreamReader]::new($qf);try{$content=$qr.ReadToEnd()}finally{$qr.Dispose()}}catch{Start-Sleep -Milliseconds 10;continue};$rows=$content.Split([char]10);while($sent -lt ($rows.Length-1)){if($rows[$sent].Trim()){$cmd=($rows[$sent]|ConvertFrom-Json).command;$out=[System.Text.Encoding]::UTF8.GetBytes($cmd+"`n");$pipe.Write($out,0,$out.Length);$pipe.Flush()};$sent++}}
 if($PollPerformance -and (([DateTime]::UtcNow-$perfLast).TotalMilliseconds -ge 1000)){$out=[Text.Encoding]::UTF8.GetBytes("cat /run/nova/performance.state`n");$pipe.Write($out,0,$out.Length);$pipe.Flush();$perfLast=[DateTime]::UtcNow}
 Start-Sleep -Milliseconds 10
 }} finally {$log.Dispose();$pipe.Dispose()}

