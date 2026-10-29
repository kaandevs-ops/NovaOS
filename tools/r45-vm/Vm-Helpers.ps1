function Get-Week4State([string]$Exe,[string]$Id){
 $lines=& $Exe showvminfo $Id --machinereadable
 if($LASTEXITCODE -ne 0){throw 'Cannot read VM state'}
 foreach($line in $lines){if($line.StartsWith('VMState=')){return $line.Substring(8).Trim('"')}}
 throw 'VM state field is missing'
}
