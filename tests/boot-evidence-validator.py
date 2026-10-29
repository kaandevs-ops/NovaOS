#!/usr/bin/env python3
"""Host-side malformed/truncated Stage-4D evidence vectors."""
import hashlib
import importlib.util
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SPEC=importlib.util.spec_from_file_location('nova_evidence',ROOT/'scripts'/'verify-boot-evidence.py')
mod=importlib.util.module_from_spec(SPEC); SPEC.loader.exec_module(mod)
H='0'*64
PCR14_AFTER=hashlib.sha256(bytes.fromhex(H)+bytes.fromhex(H)).hexdigest()
FIELDS={
 'schema':'3','evidence_kind':'novaos-boot-evidence','evidence_status':'VERIFIED','evidence_error':'none','version':'0.0.11',
 'boot_stage':'READY','normal_boot_authorized':'yes','slot':'B','slot_descriptor_format':'5','slot_descriptor_kind':'system-update',
 'rollback_index':'1','tpm_rollback_counter':'1','trust_epoch':'2','trust_epoch_state':'COMMITTED','tpm_trust_epoch_counter':'1',
 'secure_boot':'enabled','firmware_trust':'VERIFIED','release_policy_verified':'yes','release_policy_format':'3','release_policy_state':'stable',
 'release_policy_sha256':H,'preboot_release_policy_verified':'yes','preboot_release_policy_sha256':H,
 'release_key_id':'release-b','release_active_cert_sha256':H,'release_next_key_id':'none','release_next_cert_sha256':'none',
 'release_revoked_key_ids':'release-a','system_root':H,'recovery_root':H,'event_log_sha256':H,'event_log_bytes':'4096',
 'event_log_events':'20','event_log_pcr7_events':'4','event_log_error':'none','event_log_replay':'VERIFIED',
 'pcr7_live':H,'pcr7_source':'sysfs','pcr7_replayed':H,'pcr14_before':H,'pcr14_before_source':'sysfs',
 'event_digest':H,'pcr14_after':PCR14_AFTER,'pcr14_after_source':'sysfs',
}

def render(d): return ''.join(f'{k}={d[k]}\n' for k in mod.SCHEMA3_FIELDS)
mod.parse_evidence(render(FIELDS))

def reject(text, name):
    try: mod.parse_evidence(text)
    except mod.EvidenceError: return
    raise AssertionError(f'{name}: malformed evidence unexpectedly accepted')

reject(render(FIELDS).replace('schema=3\n','schema=3\nschema=3\n',1),'duplicate')
reject(render(FIELDS)+'future_field=x\n','unknown-field')
reject(render(FIELDS).rsplit('pcr14_after_source=',1)[0],'truncated')
bad=dict(FIELDS); bad['normal_boot_authorized']='no'; reject(render(bad),'verified-not-authorized')
bad=dict(FIELDS); bad['preboot_release_policy_sha256']='1'*64; reject(render(bad),'preboot-policy-mismatch')
bad=dict(FIELDS); bad['pcr14_after']='1'*64; reject(render(bad),'pcr14-relationship-mismatch')
bad=dict(FIELDS); bad['trust_epoch']='4'; reject(render(bad),'trust-epoch-jump')
bad=dict(FIELDS); bad['evidence_status']='REJECTED'; bad['normal_boot_authorized']='yes'; bad['evidence_error']='event-log-invalid'; reject(render(bad),'rejected-authorized')
rejected=dict(FIELDS); rejected['evidence_status']='REJECTED'; rejected['evidence_error']='event-log-invalid'; rejected['boot_stage']='RECOVERY_REQUIRED'; rejected['normal_boot_authorized']='no'; rejected['event_log_error']='event2-data-short'; rejected['event_log_replay']='REJECTED'; mod.parse_evidence(render(rejected))
bad=dict(rejected); bad['event_log_sha256']='unavailable'; reject(render(bad),'readable-rejected-log-missing-forensic-digest')
print('Stage-4D strict boot-evidence schema + malformed/truncated consumer rejection + forensic provenance vectors passed.')
