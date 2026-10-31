#!/usr/bin/env python3
"""Reproducible host contract runs. These are never MCU timing evidence."""
from pathlib import Path
import hashlib, json, os, platform, re, subprocess
from datetime import datetime, timezone
repo=Path(__file__).resolve().parents[1]
out=repo/'docs/metrics/host-model'; out.mkdir(parents=True,exist_ok=True)
with (out/'regression.log').open('w') as stream:
    subprocess.run(['make','test'],cwd=repo,stdout=stream,stderr=subprocess.STDOUT,check=True)
binary=repo/'tests/build'/f'{platform.system()}-{platform.machine()}'/'runtime'
rows=[]
for case in ['S1','S2','S3','RATE=40','RATE=50','RATE=60','RATE=80','RATE=120','RATE=200']:
    path=out/(case.replace('=','-')+'.log')
    env=dict(os.environ,CS_SCENARIO=case)
    with path.open('w') as stream:
        result=subprocess.run([str(binary)],env=env,stdout=stream,stderr=subprocess.STDOUT)
    text=path.read_text()
    memory=re.findall(r'^MEMORY (.*)$',text,re.M)
    rows.append(dict(case=case,exit=result.returncode,log=path.name,
                     sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                     counts=re.findall(r'^HOST_COUNTS (.*)$',text,re.M),memory=memory,
                     model_only=True))
    print(case,'exit',result.returncode, memory[-1:] or text[-150:])
manifest=dict(utc=datetime.now(timezone.utc).isoformat(),platform=platform.platform(),
    command=['python3','tools/lab3_host.py'],binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
    source_sha256={str(p.relative_to(repo)):hashlib.sha256(p.read_bytes()).hexdigest()
        for folder in ['firmware/Board','firmware/Policy','firmware/Sensors','tests']
        for p in (repo/folder).rglob('*') if p.suffix in ['.c','.h'] and 'build' not in p.parts and 'coverage' not in p.parts},
    assumptions='Virtual ms clock; RAM flash; UART TX duration ceil(N*10*1000/115200) ms; RX commands injected as bursts; S3 erase hook advances virtual clock 40 ms and injects PIR/UART at +20 ms. No real power, no MCU, no measured CPU/stack.',
    runs=rows)
(out/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
if any(r['exit'] for r in rows):raise SystemExit(1)
