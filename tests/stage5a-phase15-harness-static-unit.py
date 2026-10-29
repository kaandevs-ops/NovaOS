#!/usr/bin/env python3
import ast
import builtins
import symtable
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
H=ROOT/'scripts/windows/stage5a-phase15-check.py'
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
        for child in scope.get_children():
            walk(child)
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
assert 'SCREEN_CLIP' not in SOURCE, 'stale/undefined SCREEN_CLIP typo present'
assert "SCREEN_XCLIP=OUT/'stage5a-phase15-cross-client-clipboard.ppm'" in SOURCE
assert 'def require(' not in SOURCE, 'redundant transcript require() gate must remain removed'
assert "nova5a15-tpm-" in SOURCE and "nova5a15-mon-" in SOURCE, 'Phase-15 runtime sockets must use the Phase-15 namespace'
assert "nova5a14-" not in SOURCE, 'stale Phase-14 runtime socket namespace remains'
assert 'sel.close()' in SOURCE, 'selector must be closed during runtime cleanup'
assert "if __name__=='__main__':" in SOURCE
assert 'sys.exit(main())' in SOURCE

main=function(tree,'main')
run_assign=False
report_after=False
for node in ast.walk(main):
    if isinstance(node,(ast.Assign,ast.AnnAssign)):
        value=node.value
        if isinstance(value,ast.Call) and isinstance(value.func,ast.Name) and value.func.id=='run':
            run_assign=True
    if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and node.func.id=='report_success':
        if len(node.args)==1 and isinstance(node.args[0],ast.Name) and node.args[0].id=='shots':
            report_after=True
assert run_assign, 'main() must execute run() and retain its result'
assert report_after, 'main() must report success from the run() screenshot result'

# Serial console is diagnostic after the prompt. Runtime readiness/acceptance
# must not depend on exact concurrent service log strings.
run_fn=function(tree,'run')
for node in ast.walk(run_fn):
    if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and node.func.id=='wait_for':
        for part in ast.walk(node):
            if isinstance(part,ast.Constant) and isinstance(part.value,(bytes,str)):
                value=part.value.decode(errors='replace') if isinstance(part.value,bytes) else part.value
                assert '[nova-' not in value, f'fragile service-log wait_for gate remains: {value}'
assert 'process_lifecycle=GRACEFUL_EXIT' not in SOURCE, 'graceful acceptance must use app-authored machine state, not serial log text'
assert "x.get('stage')=='STOPPED'" in SOURCE
assert "state_int(x,'app_launch_resident_fallbacks')==0" in SOURCE

# Negative self-tests: this test must catch the exact classes of R2 defects.
bad_orchestration=SOURCE.replace('_text,shots=run()','_text,_shots=run()',1)
assert undefined_global_references(bad_orchestration), 'static lint failed to catch missing shots orchestration'
bad_screen=SOURCE.replace("('cross-client clipboard',SCREEN_XCLIP)","('cross-client clipboard',SCREEN_CLIP)",1)
assert any(name=='SCREEN_CLIP' for _,name in undefined_global_references(bad_screen)), 'static lint failed to catch undefined screenshot constant'

print('Stage-5A Phase-15 harness main/orchestration/undefined-name/no-fragile-log static unit: PASS')
