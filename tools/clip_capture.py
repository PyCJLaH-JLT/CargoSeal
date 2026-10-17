#!/usr/bin/env python3
"""Copy a time window of real scalar transitions, retaining its initial levels."""
import argparse, hashlib, json
from pathlib import Path
from read_capture import read_vcd, at
p=argparse.ArgumentParser()
p.add_argument('source',type=Path);p.add_argument('output',type=Path)
p.add_argument('--start-ms',type=float,required=True);p.add_argument('--end-ms',type=float,required=True)
p.add_argument('--channels',help='comma-separated observed channel names')
a=p.parse_args();start=round(a.start_ms*1e6);end=round(a.end_ms*1e6)
assert 0<=start<end
traces=read_vcd(a.source);symbols={name:chr(33+i) for i,name in enumerate(traces)}
if a.channels:
    traces={n:traces[n] for n in a.channels.split(',')}
    symbols={name:chr(33+i) for i,name in enumerate(traces)}
lines=['$version CargoSeal observed VCD clip $end','$timescale 1 ns $end','$scope module logic $end']
lines += [f'$var wire 1 {symbols[n]} {n} $end' for n in traces]
lines += ['$upscope $end','$enddefinitions $end','#0']
lines += [f'{at(t,start)}{symbols[n]}' for n,t in traces.items()]
edges=sorted((int(tm-start),symbols[n],v) for n,t in traces.items() for tm,v in t if start<tm<=end)
previous=None
for tm,s,v in edges:
    if tm!=previous:lines.append(f'#{tm}');previous=tm
    lines.append(f'{v}{s}')
lines.append(f'#{end-start}')
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(lines)+'\n')
a.output.with_suffix('.origin.json').write_text(json.dumps({'source':a.source.name,'source_sha256':hashlib.sha256(a.source.read_bytes()).hexdigest(),'start_ns':start,'end_ns':end,'output_sha256':hashlib.sha256(a.output.read_bytes()).hexdigest()},indent=2)+'\n')
