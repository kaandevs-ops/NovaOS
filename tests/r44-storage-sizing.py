from pathlib import Path
import importlib.util
r=Path(__file__).resolve().parents[1]
def load(name):
 s=importlib.util.spec_from_file_location(name,r/('tools/'+name+'.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
esp=load('esp-size');slots=load('system-slot-size');M=esp.M
for n in (1,16*M,48*M,64*M,92335550,128*M,496*M):
 out=esp.esp_size(n);assert out% (32*M)==0 and out>=n+16*M and 64*M<=out<=512*M
for bad in (0,-1,496*M+1,2**64):
 try:esp.esp_size(bad);raise AssertionError('unbounded ESP')
 except ValueError:pass
assert slots.system_sizes(8*M,20)==(40,48)
for n in (0,16*M,48*M,100*M,400*M):
 d,s=slots.system_sizes(n,200);assert d*M>=n+16*M and s==d+8 and d<=512
for bad in (-1,512*M,2**64):
 try:slots.system_sizes(bad,1);raise AssertionError('unbounded System')
 except ValueError:pass
assert slots.data_size(48*M)==384 and slots.data_size(136*M)==512
try:slots.data_size(2**64);raise AssertionError('unbounded Data')
except ValueError:pass
print('R44_STORAGE_SIZING_PASS legacy minima, larger payloads, margins and upper-bound failures')
