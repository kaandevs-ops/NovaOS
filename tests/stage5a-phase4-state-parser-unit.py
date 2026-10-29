#!/usr/bin/env python3
import ast
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'scripts/windows/stage5a-phase4-check.py').read_text()
tree=ast.parse(source)
names={'parse_state_chunk','state_int','raw_state_values_text','latest_raw_state_text'}
nodes=[n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name in names]
missing=names-{n.name for n in nodes}
if missing: raise SystemExit(f'missing production parser functions: {sorted(missing)}')
ns={'re':re}
exec(compile(ast.Module(body=nodes,type_ignores=[]),'<stage5a-phase4-parser>', 'exec'),ns)
chunk=(
    b'[nova-compositor] visual_generation=' + b'garbled-by-concurrent-log\n'
    b'stage=DISPLAY_READY\n'
    b'visual_generation=7\n'
    b'[nova-compositor] page_flip_count=99\n'
    b'presented_visual_generation=6\n'
    b'scene_generation=3\n'
    b'page_flip_count=12\n'
    b'nova> '
)
state=ns['parse_state_chunk'](chunk)
assert state['visual_generation']=='7',state
assert state['presented_visual_generation']=='6',state
assert state['scene_generation']=='3',state
assert ns['state_int'](state,'page_flip_count')==12,state
text=(chunk+b'\nvisual_generation=8\npresented_visual_generation=8\npage_flip_count=13\n').decode()
assert ns['raw_state_values_text'](text,'visual_generation')==[7,8]
assert ns['latest_raw_state_text'](text,'page_flip_count')==13
print('Stage-5A Phase-4 machine-state parser interleaved-log runtime unit: PASS')
