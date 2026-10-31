#!/usr/bin/env python3
"""Analyze only an actual CargoSeal VCD. Does not synthesize signals."""
import argparse, bisect, hashlib, json, re, statistics
from pathlib import Path
from read_capture import read_vcd, uart, uart_lines, pulses

def summary(values):
    if not values:return dict(n=0,min=None,avg=None,max=None,p99=None)
    ordered=sorted(values)
    return dict(n=len(values),min=min(values),avg=statistics.mean(values),max=max(values),
                p99=ordered[min(len(ordered)-1,(99*len(ordered)+99)//100-1)])

def analyze(path,installed_only=False):
    traces=read_vcd(path)
    lines=uart_lines(uart(traces['UART_TX'])) if 'UART_TX' in traces else []
    result=dict(source=str(path.resolve()),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                channels=list(traces),scope='Wokwi signal time, never physical hardware timing',
                uart_lines=lines,metrics={},firmware_stats=[x['text'] for x in lines if x['text'].startswith(('STAT ','MEMORY ','METRICS ','LOG '))])
    for name,period in [('ADC_MARK',50000),('IMU_MARK',20000),('FLASH_MARK',None),('LOOP_MARK',None)]:
        if name not in traces:continue
        points=pulses(traces[name]);widths=[p['width_ns']/1000 for p in points]
        periods=[(b['start_ns']-a['start_ns'])/1000 for a,b in zip(points,points[1:])]
        m=dict(width_us=summary(widths),period_us=summary(periods))
        if period:m.update(jitter_us=summary([v-period for v in periods]),intervals_over_2T=sum(v>period*2 for v in periods))
        result['metrics'][name]=m
    if installed_only and 'PIR' in traces:
        rises=[t for t,v in traces['PIR'] if v];used=set();pairs=[]
        for line in lines:
            if ' event=NEARBY ' not in line['text']:continue
            i=bisect.bisect_right(rises,line['time_ns'])-1
            if i<0 or i in used:continue
            used.add(i);start=rises[i]
            complete=line['time_ns']+(len(line['text'])+2)*10e9/115200
            pairs.append(dict(edge_ns=start,line_start_ns=line['time_ns'],complete_ns=complete,
                              latency_start_us=(line['time_ns']-start)/1000,
                              latency_complete_us=(complete-start)/1000))
        result['pir_pairs']=pairs
        result['metrics']['pir_to_event_complete_us']=summary([p['latency_complete_us'] for p in pairs])
        result['pir_pairing_assumption']='All observed PIR edges were in INSTALLED with no mode transitions that emit NEARBY.'
    if 'UART_RX' in traces:
        received=uart(traces['UART_RX']);commands=[];buf=bytearray();after_cr=False
        for b in received:
            value=b['byte']
            if after_cr and value==10:after_cr=False;continue
            after_cr=False
            if value in [10,13]:
                commands.append((b['time_ns']+10e9/115200,buf.decode('ascii','replace')))
                buf.clear();after_cr=value==13
            else:buf.append(value)
        pairs=[];used=set()
        for end,cmd in commands:
            prefix='STATUS ' if cmd=='STATUS' else 'OK MODE ' if cmd.startswith('MODE ') else None
            if prefix is None:continue
            for i,line in enumerate(lines):
                if i not in used and line['time_ns']>=end and line['text'].startswith(prefix):
                    used.add(i)
                    latency=(line['time_ns']+(len(line['text'])+2)*10e9/115200-end)/1000
                    pairs.append(dict(command=cmd,delimiter_end_ns=end,response_start_ns=line['time_ns'],complete_us=latency));break
        result['command_pairs']=pairs
        result['metrics']['delimiter_to_complete_us']=summary([p['complete_us'] for p in pairs])
        result['command_pairing_note']='FIFO pairing; inspect losses/overlapping diagnostics before claiming a deadline.'
    adc_stat=[x['text'] for x in lines if x['text'].startswith('STAT name=adc ')]
    if adc_stat and 'ADC_MARK' in result['metrics']:
        result['method_comparison']=dict(firmware=adc_stat[-1],gpio=result['metrics']['ADC_MARK'],
            caveat='Compare only with matching reset/run bounds. GPIO spans adc_read; timer includes call/probe overhead. Startup ADC has no STAT entry.')
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('vcd',type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--pir-installed-only',action='store_true')
    args=parser.parse_args();data=analyze(args.vcd,args.pir_installed_only)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(data['metrics'],ensure_ascii=False,indent=2))
