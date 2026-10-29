#!/usr/bin/env python3
"""Execute the production Stage-4D measurement transcript verifier in isolation."""
import ast
import contextlib
import hashlib
import io
import re
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SRC=(ROOT/'scripts/windows/final-stage4d-check.py').read_text(encoding='utf-8')
tree=ast.parse(SRC)
fn=next((n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='verify_measurement'),None)
if fn is None:
    raise SystemExit('production verify_measurement() not found')
module=ast.Module(body=[fn],type_ignores=[])
ast.fix_missing_locations(module)
ns={'hashlib':hashlib,'re':re,'sys':sys}
exec(compile(module,'<production-verify_measurement>','exec'),ns)
verify=ns['verify_measurement']

H='11'*32
P0='00'*32
fields={
    'measurement_format':'2','evidence_schema':'3','version':'0.0.11','pcr':'14','algorithm':'sha256',
    'slot':'A','slot_descriptor_format':'4','slot_descriptor_kind':'base','rollback_index':'0','tpm_rollback_counter':'0',
    'system_root':H,'recovery_root':H,'secure_boot':'enabled','firmware_trust':'VERIFIED','pcr7':H,
    'event_log_sha256':H,'event_log_replayed_pcr7':H,'release_policy_sha256':H,
    'preboot_release_policy_sha256':'unavailable','release_active_cert_sha256':H,
    'release_key_id':'nova-development-release-2026-a','trust_epoch':'1','tpm_trust_epoch_counter':'0',
    'uefi_db_cert_sha256':H,'pcr14_before':P0,
}
preimage=(
    'NovaOS Stage-4D measured boot evidence v2\n'
    f"evidence_schema={fields['evidence_schema']}\nversion={fields['version']}\nslot={fields['slot']}\n"
    f"slot_descriptor_format={fields['slot_descriptor_format']}\nslot_descriptor_kind={fields['slot_descriptor_kind']}\n"
    f"rollback_index={fields['rollback_index']}\ntpm_rollback_counter={fields['tpm_rollback_counter']}\n"
    f"system_root={fields['system_root']}\nrecovery_root={fields['recovery_root']}\nsecure_boot={fields['secure_boot']}\n"
    f"firmware_trust={fields['firmware_trust']}\npcr7={fields['pcr7']}\nevent_log_sha256={fields['event_log_sha256']}\n"
    f"event_log_replayed_pcr7={fields['event_log_replayed_pcr7']}\nrelease_policy_sha256={fields['release_policy_sha256']}\n"
    f"preboot_release_policy_sha256={fields['preboot_release_policy_sha256']}\nrelease_active_cert_sha256={fields['release_active_cert_sha256']}\n"
    f"release_key_id={fields['release_key_id']}\ntrust_epoch={fields['trust_epoch']}\ntpm_trust_epoch_counter={fields['tpm_trust_epoch_counter']}\n"
    f"uefi_db_cert_sha256={fields['uefi_db_cert_sha256']}\n"
).encode('ascii')
fields['event_digest']=hashlib.sha256(preimage).hexdigest()
fields['pcr14_after']=hashlib.sha256(bytes.fromhex(P0)+bytes.fromhex(fields['event_digest'])).hexdigest()
order=(
    'measurement_format','evidence_schema','version','pcr','algorithm','slot','slot_descriptor_format','slot_descriptor_kind',
    'rollback_index','tpm_rollback_counter','system_root','recovery_root','secure_boot','firmware_trust','pcr7',
    'event_log_sha256','event_log_replayed_pcr7','release_policy_sha256','preboot_release_policy_sha256',
    'release_active_cert_sha256','release_key_id','trust_epoch','tpm_trust_epoch_counter','uefi_db_cert_sha256',
    'pcr14_before','event_digest','pcr14_after'
)
record='\n'.join(f'{k}={fields[k]}' for k in order)+'\n'
# Deliberately append conflicting fields from a later state/confirm command.
# A whole-transcript last-match parser would consume these and break the preimage.
transcript='stage=READY\n'+record+'tpm_rollback_counter=99\ntrust_epoch=77\npcr14_after='+'ff'*32+'\n'
if verify(transcript,'runtime-unit') is not True:
    raise SystemExit('bounded production measurement parser rejected a valid record')

err=io.StringIO()
with contextlib.redirect_stderr(err):
    missing=verify('stage=READY\ntpm_rollback_counter=0\n','missing-record')
if missing is not False or 'missing measurement_format' not in err.getvalue():
    raise SystemExit('production verifier did not reject a transcript missing measurements.log')

print('Stage-4D bounded production measurement transcript runtime unit: PASS')
