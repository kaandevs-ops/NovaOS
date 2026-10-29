#!/usr/bin/env python3
import ast,builtins,symtable
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
H=ROOT/'scripts/windows/stage5a-phase17-check.py'
SOURCE=H.read_text(encoding='utf-8')

def undefined_global_references(source):
    table=symtable.symtable(source,str(H),'exec')
    module_defs={s.get_name() for s in table.get_symbols() if s.is_assigned() or s.is_imported() or s.is_namespace()}
    allowed=set(dir(builtins))|{'__file__','__name__'}; missing=[]
    for sym in table.get_symbols():
        if sym.is_referenced() and sym.get_name() not in module_defs and sym.get_name() not in allowed: missing.append(('module',sym.get_name()))
    def walk(scope):
        for sym in scope.get_symbols():
            if sym.is_global() and sym.is_referenced() and sym.get_name() not in module_defs and sym.get_name() not in allowed:
                missing.append((scope.get_name(),sym.get_name()))
        for child in scope.get_children(): walk(child)
    walk(table); return sorted(set(missing))

def function(tree,name):
    for n in tree.body:
        if isinstance(n,ast.FunctionDef) and n.name==name: return n
    raise AssertionError(f'missing function: {name}')

tree=ast.parse(SOURCE,str(H)); missing=undefined_global_references(SOURCE)
assert not missing, f'undefined global reference(s): {missing}'
assert 'nova5a17-tpm-' in SOURCE and 'nova5a17-mon-' in SOURCE
assert 'nova5a16-' not in SOURCE
assert "TRANSCRIPT=OUT/'stage5a-phase17.log'" in SOURCE
assert "PROBE_A_STATE='/Data/Nova/Apps/nova.sandbox.probe.a/phase17-sandbox.state'" in SOURCE
assert "PROBE_B_STATE='/Data/Nova/Apps/nova.sandbox.probe.b/phase17-sandbox.state'" in SOURCE
assert "SANDBOX_IPC_STATE_PATH='/run/nova/sandbox-ipc.state'" in SOURCE
assert 'sel.close()' in SOURCE
assert "if __name__=='__main__':" in SOURCE and 'sys.exit(main())' in SOURCE
main=function(tree,'main'); run_assign=False; report=False
for node in ast.walk(main):
    if isinstance(node,(ast.Assign,ast.AnnAssign)) and isinstance(node.value,ast.Call) and isinstance(node.value.func,ast.Name) and node.value.func.id=='run': run_assign=True
    if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and node.func.id=='report_success': report=True
assert run_assign and report

# Acceptance authority is machine state + /proc, never service-log substrings.
run_fn=function(tree,'run')
for node in ast.walk(run_fn):
    if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and node.func.id=='wait_for':
        for part in ast.walk(node):
            if isinstance(part,ast.Constant) and isinstance(part.value,(bytes,str)):
                v=part.value.decode(errors='replace') if isinstance(part.value,bytes) else part.value
                assert '[nova-' not in v, f'fragile service-log gate: {v}'
for marker in (
    "st.get('NoNewPrivs')!='1'","st.get('Seccomp')!='2'",
    "slot_value(x,3,'peer_pid')","slot_value(x,4,'peer_pid')",
    "service kill phase17-sandbox-probe-a","service kill compositor",
    "persistent_preexisting')!=persistent_preexisting","state_int(state,'tmp_preexisting')!=0",
): assert marker in SOURCE, f'missing Phase-17 machine-state/runtime marker: {marker}'

bad=SOURCE.replace('_text,shots=run()','_text,_shots=run()',1)
assert undefined_global_references(bad), 'undefined-name lint failed'
bad2=SOURCE.replace("'legacy':SCREEN_LEGACY","'legacy':SCREEN_LEGAC",1)
assert any(name=='SCREEN_LEGAC' for _,name in undefined_global_references(bad2)), 'screenshot typo lint failed'
assert "persistent_previous_pid" in SOURCE
assert "wait_probe_ready(proc,buf,sel,PROBE_A_STATE,a_pid2,3" in SOURCE
assert "wait_probe_ready(proc,buf,sel,PROBE_A_STATE,a_pid,3" in SOURCE
assert "wait_probe_ready(proc,buf,sel,PROBE_B_STATE,b_pid,4" in SOURCE
assert "connect_attempts" in SOURCE and "connect_errno" in SOURCE and "socket_errno" in SOURCE
assert "app-facing compositor IPC listener" in SOURCE
print('Stage-5A Phase-17 harness orchestration/undefined-name/proc+machine-state authority static unit: PASS')
