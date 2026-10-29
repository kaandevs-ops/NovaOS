#!/usr/bin/env python3
import ast
import builtins
import symtable
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
H=ROOT/'scripts/windows/stage5a-phase16-check.py'
SOURCE=H.read_text(encoding='utf-8')


def undefined_global_references(source):
    table=symtable.symtable(source,str(H),'exec')
    module_defs={
        sym.get_name() for sym in table.get_symbols()
        if sym.is_assigned() or sym.is_imported() or sym.is_namespace()
    }
    allowed=set(dir(builtins))|{'__file__','__name__'}
    missing=[]
    for sym in table.get_symbols():
        if sym.is_referenced() and sym.get_name() not in module_defs and sym.get_name() not in allowed:
            missing.append(('module',sym.get_name()))
    def walk(scope):
        for sym in scope.get_symbols():
            if sym.is_global() and sym.is_referenced() and sym.get_name() not in module_defs and sym.get_name() not in allowed:
                missing.append((scope.get_name(),sym.get_name()))
        for child in scope.get_children(): walk(child)
    walk(table)
    return sorted(set(missing))


def function(tree,name):
    for node in tree.body:
        if isinstance(node,ast.FunctionDef) and node.name==name:
            return node
    raise AssertionError(f'missing function: {name}')


tree=ast.parse(SOURCE,str(H))
missing=undefined_global_references(SOURCE)
assert not missing, f'undefined global reference(s): {missing}'
assert "nova5a16-tpm-" in SOURCE and "nova5a16-mon-" in SOURCE
assert "nova5a15-" not in SOURCE, 'stale Phase-15 runtime socket namespace remains'
assert "TRANSCRIPT=OUT/'stage5a-phase16.log'" in SOURCE
assert 'sel.close()' in SOURCE
assert "if __name__=='__main__':" in SOURCE and 'sys.exit(main())' in SOURCE
assert 'def require(' not in SOURCE

main=function(tree,'main')
run_assign=False; report=False
for node in ast.walk(main):
    if isinstance(node,(ast.Assign,ast.AnnAssign)):
        value=node.value
        if isinstance(value,ast.Call) and isinstance(value.func,ast.Name) and value.func.id=='run': run_assign=True
    if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and node.func.id=='report_success': report=True
assert run_assign and report

# Runtime acceptance authority must be machine state, not concurrent serial-log text.
run_fn=function(tree,'run')
for node in ast.walk(run_fn):
    if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and node.func.id=='wait_for':
        for part in ast.walk(node):
            if isinstance(part,ast.Constant) and isinstance(part.value,(bytes,str)):
                value=part.value.decode(errors='replace') if isinstance(part.value,bytes) else part.value
                assert '[nova-' not in value, f'fragile service-log wait gate remains: {value}'

for marker in (
    "state_int(x,'client_id_mismatch_denials')",
    "state_int(x,'ui_surface_denials')",
    "state_int(x,'principal_lookup_denials')",
    "state_int(x,'input_text_denials')",
    "state_int(x,'tab_capture_denials')",
    "state_int(x,'clipboard_read_denials')",
    "state_int(x,'clipboard_write_denials')",
    "slot_value(x,3,'peer_pid')",
    "slot_value(x,3,'generation')",
    "slot_value(x,5,'capabilities')",
):
    assert marker in SOURCE, f'missing machine-state acceptance marker: {marker}'

# Catch orchestration/name regressions of the same class as Phase-15 R2.
bad=SOURCE.replace('_text,shots=run()','_text,_shots=run()',1)
assert undefined_global_references(bad), 'undefined-name static lint failed to catch lost shots variable'
bad2=SOURCE.replace("'reauth': SCREEN_REAUTH", "'reauth': SCREEN_REAUT",1)
assert any(name=='SCREEN_REAUT' for _,name in undefined_global_references(bad2)), 'undefined screenshot typo was not caught'

print('Stage-5A Phase-16 harness orchestration/undefined-name/machine-state-authority static unit: PASS')
