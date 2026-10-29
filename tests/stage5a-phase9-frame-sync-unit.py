#!/usr/bin/env python3
"""Exercise the exact Phase-9 host commit-to-presented-frame predicate.

The critical regression is queued client DAMAGE/COMMIT traffic: the host must
wait for the exact client-reported COMMIT target for the action under test, not
merely compositor_before+1.
"""
import ast
from pathlib import Path

source = Path(__file__).resolve().parents[1] / 'scripts/windows/stage5a-phase9-check.py'
tree = ast.parse(source.read_text(encoding='utf-8'), filename=str(source))
names = {'state_int', 'client_commit_target_presented'}
functions = [node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name in names]
assert {node.name for node in functions} == names
scope = {'re': __import__('re')}
exec(compile(ast.Module(body=functions, type_ignores=[]), str(source), 'exec'), scope)
ready = scope['client_commit_target_presented']

visual_before = 22
exact_target = 19

# Compositor is several commits behind the app.  R2's before+1-style barrier
# would have accepted one of these older queued commits even though the TAB
# redraw (exact_target=19) had not been consumed yet.
queued_old_presented = {'client_commits':'15', 'visual_generation':'27', 'presented_visual_generation':'27'}
assert not ready(queued_old_presented, exact_target, visual_before), 'older queued commit was mistaken for exact action commit'

exact_pending = {'client_commits':'19', 'visual_generation':'31', 'presented_visual_generation':'30'}
assert not ready(exact_pending, exact_target, visual_before), 'exact commit was accepted before presentation'

no_new_visual = {'client_commits':'19', 'visual_generation':'22', 'presented_visual_generation':'22'}
assert not ready(no_new_visual, exact_target, visual_before), 'exact commit without a new visual generation was accepted'

exact_presented = {'client_commits':'19', 'visual_generation':'31', 'presented_visual_generation':'31'}
assert ready(exact_presented, exact_target, visual_before), 'exact client commit + presented frame was rejected'

later_presented = {'client_commits':'20', 'visual_generation':'32', 'presented_visual_generation':'32'}
assert ready(later_presented, exact_target, visual_before), 'later frame containing exact target was rejected'
assert not ready(exact_presented, 0, visual_before), 'missing exact target was accepted'
print('Stage-5A Phase-9 exact client-commit backlog/presented-frame synchronization runtime unit: PASS')
