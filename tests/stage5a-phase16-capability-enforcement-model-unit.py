#!/usr/bin/env python3
UI=1<<0
TEXT=1<<1
TAB=1<<2
CB_READ=1<<3
CB_WRITE=1<<4
KNOWN=UI|TEXT|TAB|CB_READ|CB_WRITE

F_TAB=1<<0
F_TEXT=1<<1
F_CLIP=1<<2

class Policy:
    def __init__(self,caps):
        assert caps & ~KNOWN == 0
        self.caps=caps
        self.denials=[]
    def has(self,cap):
        return (self.caps & cap)==cap
    def deny(self,name):
        self.denials.append(name)
        return False
    def set_input_flags(self,flags):
        if flags & F_TAB and not self.has(TAB): return self.deny('input.tab-capture')
        if flags & F_TEXT and not self.has(TEXT): return self.deny('input.text')
        if flags & F_CLIP and not (self.has(CB_READ) or self.has(CB_WRITE)):
            return self.deny('clipboard.read|clipboard.write')
        return True
    def route_key(self,key):
        if key=='TAB':
            return self.has(TAB) or self.deny('input.tab-capture')
        return self.has(TEXT) or self.deny('input.text')
    def clipboard_set(self):
        return self.has(CB_WRITE) or self.deny('clipboard.write')
    def clipboard_get(self):
        return self.has(CB_READ) or self.deny('clipboard.read')

full=Policy(KNOWN)
assert full.set_input_flags(F_TAB|F_TEXT|F_CLIP)
assert full.route_key('A') and full.route_key('TAB')
assert full.clipboard_set() and full.clipboard_get()
assert not full.denials

# Clipboard authority is intentionally asymmetric.
read_only=Policy(UI|CB_READ)
assert read_only.set_input_flags(F_CLIP)
assert read_only.clipboard_get()
assert not read_only.clipboard_set()
assert read_only.denials==['clipboard.write']

write_only=Policy(UI|CB_WRITE)
assert write_only.set_input_flags(F_CLIP)
assert write_only.clipboard_set()
assert not write_only.clipboard_get()
assert write_only.denials==['clipboard.read']

no_clip=Policy(UI|TEXT|TAB)
assert not no_clip.set_input_flags(F_CLIP)
assert not no_clip.clipboard_get()
assert not no_clip.clipboard_set()

# Text input and TAB capture are separate decisions.
text_only=Policy(UI|TEXT)
assert text_only.set_input_flags(F_TEXT)
assert text_only.route_key('A')
assert not text_only.set_input_flags(F_TAB)
assert not text_only.route_key('TAB')

tab_only=Policy(UI|TAB)
assert tab_only.set_input_flags(F_TAB)
assert tab_only.route_key('TAB')
assert not tab_only.set_input_flags(F_TEXT)
assert not tab_only.route_key('A')

# Denying one capability must not revoke unrelated allowed capabilities.
mixed=Policy(UI|TEXT|CB_READ)
assert mixed.route_key('A')
assert mixed.clipboard_get()
assert not mixed.clipboard_set()
assert mixed.route_key('A')
assert mixed.clipboard_get()

print('Stage-5A Phase-16 independent input/TAB/clipboard read-write capability enforcement model unit: PASS')
