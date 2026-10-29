#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
start=s.index('static long load_or_reuse_release_key')
end=s.index('static int verify_release_policy_bundle',start)
helper=s[start:end]
# Certificate bytes are never reused on ID alone: the file hash must first
# equal the root-authenticated policy digest.
assert 'nova_sha256_file(cert_path,actual)' in helper
assert '!nova_streq(actual,expected_sha256)' in helper
# Both currently authenticated slots in the delegated-key lifecycle can be
# reused without calling add_key() again with the same description.
assert 'g_release_key>=0' in helper and 'g_release_active_key_id' in helper and 'g_release_active_cert_sha256' in helper
assert 'g_release_next_key>=0' in helper and 'g_release_next_key_id' in helper and 'g_release_next_cert_sha256' in helper
assert helper.index('g_release_key>=0') < helper.index('nova_add_key("asymmetric"')
assert helper.index('g_release_next_key>=0') < helper.index('nova_add_key("asymmetric"')
verify=s[s.index('static int verify_release_policy_bundle'):s.index('static void apply_release_policy_view')]
assert 'load_or_reuse_release_key(out->active_key_id' in verify
assert 'load_or_reuse_release_key(out->next_key_id' in verify
assert 'nova_add_key("asymmetric"' not in verify
print('Stage-4D release-key keyring serial reuse source contract passed.')
PY
