from pathlib import Path
import re,json
p=Path(__file__).resolve().parents[1]/'src/runtime/nova_r24_perf.h';s=p.read_text();size=int(re.search(r'#define R24_REPORT_BYTES (\d+)U',s)[1]);labels=re.findall(r'R24_FIELD\("([^"\n]+)"',s)
assert len(labels)==len(set(labels));payload='R24_PERF '+json.dumps({x:2**64-1 for x in labels},separators=(',',':'))+'\n'
assert len(payload)<size,(len(payload),size)
assert json.loads(payload[len('R24_PERF '):])['wallpaper_cache_bytes']==2**64-1
print(f'R24_TELEMETRY_MAX_VALUES_CLOSED_JSON_BOUND={len(payload)}/{size}_BYTES_PASS')
