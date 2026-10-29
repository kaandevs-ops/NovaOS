#!/usr/bin/env python3
from pathlib import Path
import re,json
r=Path(__file__).resolve().parents[1]
ui=(r/'files/src/nova_files_ui.c').read_text();tokens=(r/'src/runtime/nova_ui.h').read_text()
def value(name,source):return int(re.search(r'#define '+name+r' 0x([0-9a-fA-F]+)U',source).group(1),16)
def lum(c):
 v=[((c>>s)&255)/255 for s in (16,8,0)];v=[x/12.92 if x<=.04045 else ((x+.055)/1.055)**2.4 for x in v];return sum(a*b for a,b in zip(v,(.2126,.7152,.0722)))
ink=value('NOVA_UI_INK',tokens);muted=value('NOVA_UI_MUTED',tokens);selected=value('NOVA_UI_SELECTION',tokens);paper=value('C_BG',ui);sidebar=value('C_SIDEBAR',ui)
pairs={}
for name,fg,bg in [('body',ink,paper),('muted',muted,paper),('sidebar',muted,sidebar),('selected',ink,selected)]:
 a,b=sorted((lum(fg),lum(bg)));ratio=(b+.05)/(a+.05);pairs[name]={'contrast_ratio':round(ratio,3),'at_least_4_5':ratio>=4.5}
result={'scope':'specified opaque text/background token pairs only, not full accessibility certification','status':'PASS' if all(p['at_least_4_5'] for p in pairs.values()) else 'FAIL','pairs':pairs}
(r/'evidence/r18/text-contrast-final.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2));assert result['status']=='PASS'
