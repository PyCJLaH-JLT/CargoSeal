#!/usr/bin/env python3
"""Record a running CargoSeal, using MCU uptime for scenario duration.

Requires the permitted, already running Wokwi VS Code RFC2217 server or a real
serial device. This program never starts a GUI, acquires a license, or fakes input.
Sensor controls/fixtures are recorded separately. Host send times are not latency.
"""
import argparse, hashlib, json, re, time
from datetime import datetime,timezone
from pathlib import Path
import serial

parser=argparse.ArgumentParser()
parser.add_argument('scenario',choices=['bringup','S1','S2','S3','fault','rate'])
parser.add_argument('--url',default='rfc2217://127.0.0.1:43106')
parser.add_argument('--seconds',type=int)
parser.add_argument('--rate',type=float,default=20)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--conditions',required=True,help='Exact diagram, fixture, sensor conditions and platform')
args=parser.parse_args()
repo=Path(__file__).resolve().parents[1]
if not 0<args.rate<=1000:parser.error('rate must be >0 and <=1000')
seconds=args.seconds or {'bringup':10,'S1':120,'S2':3600,'S3':120,'fault':10,'rate':30}[args.scenario]
if seconds<=0:parser.error('seconds must be positive')
args.output.mkdir(parents=True,exist_ok=False)
manifest=json.loads((repo/'docs/metrics/lab3-build/manifest.json').read_text())
metadata=dict(utc=datetime.now(timezone.utc).isoformat(),scenario=args.scenario,
 conditions=args.conditions,url=args.url,requested_mcu_seconds=seconds,requested_host_rate=args.rate,
 build_id=manifest['build_id'],source_digest=manifest['source_digest'],firmware_artifacts=manifest['artifacts'],
 pyserial=serial.VERSION,clock_rule='Duration from STATUS uptime_ms; host times only describe transmission/recording.')
(args.output/'origin.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2))
raw_path=args.output/'uart.raw';commands_path=args.output/'commands.jsonl'
wall_start=time.monotonic();last_progress=wall_start;first=None;latest=None;last_change=None
verified=False;buffer=b'';state='failed';error=None;sent=0
try:
 with raw_path.open('wb') as raw,commands_path.open('w') as commands,serial.serial_for_url(args.url,baudrate=115200,timeout=.002) as port:
  def send(text):
   global sent
   data=(text+'\n').encode();port.write(data);sent+=1
   commands.write(json.dumps(dict(host_monotonic=time.monotonic(),command=text,bytes=len(data)))+'\n');commands.flush()
  send('STATS');send('LOG STATUS');send('STATUS')
  next_status=time.monotonic()+1;next_stream=None;fault_stage=0;next_stats=600
  while True:
   chunk=port.read(4096)
   if chunk:
    raw.write(chunk);raw.flush();buffer+=chunk
    while b'\n' in buffer:
     line,buffer=buffer.split(b'\n',1);text=line.decode('ascii','replace').rstrip('\r')
     if text.startswith('CLOCK_FAULT '):
      raise RuntimeError('MCU reported a backwards timebase; duration and timing metrics are invalid')
     if text.startswith('METRICS '):
      m=re.search(r'build=(\S+)',text)
      if m and m[1]!=manifest['build_id']:raise RuntimeError('MCU build ID differs from manifest')
      verified=bool(m)
     m=re.search(r'^STATUS .*uptime_ms=(\d+)',text)
     if m:
      value=int(m[1])
      if latest is not None and ((value-latest)&0xffffffff)>60000:
       raise RuntimeError('MCU uptime jumped by more than 60 s between STATUS samples')
      if latest!=value:last_change=time.monotonic()
      latest=value
      if first is None and verified:
       first=value;next_stream=time.monotonic();print('MCU start uptime_ms',first,flush=True)
   now=time.monotonic()
   if first is None:
    if now-wall_start>60:raise RuntimeError('No verified MCU metrics and advancing STATUS within 60 host seconds')
   else:
    elapsed=((latest-first)&0xffffffff)/1000
    if elapsed>=seconds:break
    if last_change is not None and now-last_change>90:raise RuntimeError('MCU time stopped advancing for 90 host seconds')
    if args.scenario in ['S3','rate'] and now>=next_stream:
     send('STATUS');next_stream+=1/args.rate
     # A paused/slow simulator must not cause an unbounded host catch-up burst.
     if now-next_stream>1:next_stream=now+1/args.rate
    if args.scenario=='fault':
     if elapsed>=2 and fault_stage==0:send('IMU ADDRESS 0x69');fault_stage=1
     if elapsed>=5 and fault_stage==1:send('IMU ADDRESS 0x68');fault_stage=2
    if args.scenario=='S2' and elapsed>=next_stats:send('STATS');send('LOG STATUS');next_stats+=600
   if now>=next_status:
    if args.scenario not in ['S3','rate']:send('STATUS')
    next_status=now+(5 if first is not None else 1)
   if now-last_progress>=60:
    print('progress mcu_seconds',None if first is None else ((latest-first)&0xffffffff)/1000,'host_seconds',round(now-wall_start),'sent',sent,flush=True)
    last_progress=now
  # Drain before diagnostic snapshot. This does not belong to the load interval.
  deadline=time.monotonic()+2
  while time.monotonic()<deadline:
   chunk=port.read(4096)
   if chunk:raw.write(chunk)
  send('STATS');send('LOG STATUS');send('LOG DUMP')
  deadline=time.monotonic()+5
  while time.monotonic()<deadline:
   chunk=port.read(4096)
   if chunk:raw.write(chunk)
  state='recorded'
except Exception as exc:
 error=str(exc)
finally:
 result=dict(state=state,error=error,verified_build=verified,first_uptime_ms=first,last_uptime_ms=latest,
  elapsed_mcu_seconds=None if first is None or latest is None else ((latest-first)&0xffffffff)/1000,
  host_seconds=time.monotonic()-wall_start,sent_commands=sent,
  raw_sha256=hashlib.sha256(raw_path.read_bytes()).hexdigest() if raw_path.exists() else None,
  note='Recorded is not an FR pass. Inspect wire captures, dropped counters, MCU stack/heap and sensor conditions.')
 (args.output/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2))
 print(json.dumps(result,ensure_ascii=False,indent=2),flush=True)
if state!='recorded':raise SystemExit(1)
