#!/usr/bin/env python3
"""Offline build tool. Pillow/FreeType only on host; generated C needs no parser/library/network."""
from pathlib import Path
from PIL import ImageFont
import hashlib,json,PIL
r=Path(__file__).resolve().parents[1];font=r/'assets/fonts/InterVariable.ttf';assert hashlib.sha256(font.read_bytes()).hexdigest()=='4989b125924991b90d05b2d16e0e388c48f7d5bb8b30539bbf9c755278d0ccaf'
codepoints=list(range(32,127))+[0x00b7,0x00c7,0x00d6,0x00dc,0x00e7,0x00f6,0x00fc,0x011e,0x011f,0x0130,0x0131,0x015e,0x015f,0x2013,0x2026];codepoints.sort()
styles=[(14,400),(20,600),(12,500),(13,600)];lines=['/* Inter OFL1.1; generated offline with build-r19-inter-atlas.py. */','#ifndef NOVA_UI_FONT_DATA_H','#define NOVA_UI_FONT_DATA_H','struct nova_ui_glyph { unsigned int cp,offset; short x,y; unsigned short w,h,advance; };','struct nova_ui_kern { unsigned short left,right; short adjust; };','struct nova_ui_face { short ascent,line; const struct nova_ui_glyph*glyphs; const unsigned char*alpha; const struct nova_ui_kern*kern; unsigned int nkern; };']
faces=[];info=[]
for scale in [1,2]:
 for role,(px,weight) in enumerate(styles):
  index=len(faces);f=ImageFont.truetype(str(font),px*scale);axes=f.get_variation_axes();values=[]
  for axis in axes:
   name=axis['name'].decode();v=weight if 'Weight' in name else min(axis['maximum'],max(axis['minimum'],px));values.append(v)
  f.set_variation_by_axes(values);ascent,descent=f.getmetrics();data=[];glyphs=[]
  for cp in codepoints:
   mask,offset=f.getmask2(chr(cp),mode='L',anchor='ls');w,h=mask.size;glyphs.append((cp,len(data),*offset,w,h,round(f.getlength(chr(cp))*64)));data.extend(bytes(mask))
  kern=[]
  for pair in ['AV','AW','AY','AT','FA','LT','LV','LY','PA','Ta','Te','To','Tr','Tu','Va','Vo','Wa','Wo','Ya','Yo']:
   adjust=round((f.getlength(pair)-f.getlength(pair[0])-f.getlength(pair[1]))*64)
   if adjust:kern.append((codepoints.index(ord(pair[0])),codepoints.index(ord(pair[1])),adjust))
  lines+=['static const unsigned char nova_ui_alpha_'+str(index)+'[] = {']
  lines += [','.join(map(str,data[i:i+48]))+',' for i in range(0,len(data),48)];lines+=['};','static const struct nova_ui_glyph nova_ui_glyphs_'+str(index)+'[] = {']
  lines += ['{'+','.join(map(str,g))+'},' for g in glyphs];lines+=['};','static const struct nova_ui_kern nova_ui_kern_'+str(index)+'[] = {']
  lines += ['{'+','.join(map(str,k))+'},' for k in kern];lines+=['};'];faces.append('{'+str(ascent)+','+str(ascent+descent)+',nova_ui_glyphs_'+str(index)+',nova_ui_alpha_'+str(index)+',nova_ui_kern_'+str(index)+','+str(len(kern))+'}')
  info.append({'role':role,'scale':scale,'pixels':px*scale,'weight':weight,'ascent':ascent,'line_height':ascent+descent,'alpha_bytes':len(data)})
lines+=['#define NOVA_UI_GLYPH_COUNT '+str(len(codepoints))+'U','static const struct nova_ui_face nova_ui_faces[8] = {'+','.join(faces)+'};','#endif']
output=r/'src/runtime/nova_ui_font_data.h';output.write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
(r/'assets/fonts/atlas-manifest.json').write_text(json.dumps({'font_url':'https://raw.githubusercontent.com/rsms/inter/master/docs/font-files/InterVariable.ttf','font_sha256':hashlib.sha256(font.read_bytes()).hexdigest(),'generator_pillow':PIL.__version__,'glyphs':len(codepoints),'scope':'ASCII + Turkish precomposed + middle dot/en dash/ellipsis, not full Unicode/shaping','faces':info,'header_sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2),encoding='utf-8')
print('Atlas generated',len(codepoints),'glyphs x 8 faces;',sum(x['alpha_bytes'] for x in info),'alpha bytes')
