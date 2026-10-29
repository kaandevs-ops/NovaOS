$ErrorActionPreference='Stop'
$root=$PSScriptRoot;$v=Join-Path $env:ProgramFiles 'Oracle\VirtualBox\VBoxManage.exe'
$identity=Get-Content -LiteralPath (Join-Path $root 'r46-vm-identity.json') -Raw|ConvertFrom-Json
$id=$identity.uuid;$name='NovaOS-R46-Desktop-Test'
if($identity.name -ne $name -or $id -ne 'afeecd0e-5b84-49d5-8cf0-f80e9b120ffa'){throw 'R46 identity guard failed'}
$folder=Join-Path $root ('VMs\'+$name)
if(Test-Path -LiteralPath $folder){throw 'Refusing to replace existing R46 VM directory'}
$list=(& $v list vms|Out-String);if($list.Contains($id)-or$list.Contains('"'+$name+'"')){throw 'R46 identity/name already registered'}
function Run([string[]]$args44){& $v @args44;if($LASTEXITCODE -ne 0){throw ('VBox command failed: '+$args44[0])}}
Run @('createvm','--name',$name,'--uuid',$id,'--ostype','Linux_64','--basefolder',(Join-Path $root 'VMs'),'--register')
Run @('modifyvm',$id,'--memory','2048','--cpus','2','--firmware','efi64','--tpm-type','2.0','--graphicscontroller','vmsvga','--vram','128','--accelerate-3d','off','--mouse','ps2','--keyboard','ps2','--ioapic','on','--nested-paging','on','--boot1','disk','--boot2','none','--boot3','none','--boot4','none','--nic1','nat','--cable-connected1','off','--uart1','0x3f8','4','--uart-mode1','server','\\.\pipe\NovaOS-R46-Console','--clipboard-mode','disabled','--drag-and-drop','disabled')
Run @('storagectl',$id,'--name','SATA','--add','sata','--controller','IntelAhci','--portcount','1','--hostiocache','off')
Run @('modifynvram',$id,'inituefivarstore')
@{uuid=$id;name=$name;path=$folder;status='CREATED_OFFLINE_NO_DISK_NO_BOOT'}|ConvertTo-Json|Set-Content -Encoding utf8 -LiteralPath (Join-Path $root 'creation-result.json')
