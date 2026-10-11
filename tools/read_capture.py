#!/usr/bin/env python3
"""Decode observed scalar VCD transitions; never generate substitute signals."""
import argparse, bisect, gzip, hashlib, json, re, statistics
from pathlib import Path
def read_vcd(path):
    traces, names, scale, now = {}, {}, 1, 0
    header = True
    source=gzip.decompress(path.read_bytes()).decode() if path.suffix==".gz" else path.read_text()
    for line in source.splitlines():
        line=line.strip()
        if header:
            m=re.match(r"\$var\s+\w+\s+1\s+(\S+)\s+(\S+)",line)
            if m:names[m[1]]=m[2];traces[m[2]]=[]
            m=re.search(r"\$timescale\s+(\d+)\s*(ns|us|ms|ps)",line)
            if m:scale=int(m[1])*{"ns":1,"us":1000,"ms":1000000,"ps":0.001}[m[2]]
            if "$enddefinitions" in line:header=False
        elif line.startswith("#"):now=int(line[1:])*scale
        elif line and line[0] in "01" and line[1:] in names:
            trace=traces[names[line[1:]]];value=int(line[0])
            if trace and trace[-1][0]==now:trace[-1]=(now,value)
            elif not trace or trace[-1][1]!=value:trace.append((now,value))
    return traces
def at(trace,time):
    i=bisect.bisect_right(trace,(time,2))-1
    return trace[i][1] if i>=0 else 1
def uart(trace,baud=115200):
    period=1e9/baud;out=[];until=-1;previous=1
    for time,value in trace:
        if previous and not value and time>=until:
            if at(trace,time+0.5*period)==0 and at(trace,time+9.5*period)==1:
                byte=sum(at(trace,time+(1.5+i)*period)<<i for i in range(8))
                out.append({"time_ns":time,"byte":byte});until=time+9.75*period
        previous=value
    return out
def uart_lines(bytes_):
    lines=[];start=None;buffer=bytearray()
    for b in bytes_:
        if start is None:start=b["time_ns"]
        if b["byte"]==10:
            lines.append({"time_ns":start,"text":buffer.decode("ascii","replace").rstrip("\r")})
            buffer.clear();start=None
        else:buffer.append(b["byte"])
    return lines
def i2c(scl,sda):
    edges=sorted([(t,"scl",v) for t,v in scl]+[(t,"sda",v) for t,v in sda])
    clock=data=1;active=False;bits=[];result=[];start=0
    for time,name,value in edges:
        if name=="sda":
            if clock and value!=data:
                if value==0:
                    if active:result.append({"time_ns":time,"kind":"RESTART"})
                    else:result.append({"time_ns":time,"kind":"START"})
                    active=True;bits=[];start=time
                else:
                    if active:result.append({"time_ns":time,"kind":"STOP","duration_ns":time-start})
                    active=False;bits=[]
            data=value
        else:
            if active and not clock and value:
                bits.append(data)
                if len(bits)==9:
                    byte=sum(bits[i]<<(7-i) for i in range(8))
                    result.append({"time_ns":time,"kind":"BYTE","hex":f"{byte:02X}","ack":not bool(bits[8])})
                    bits=[]
            clock=value
    return result
def pulses(trace):
    rising=None;out=[]
    for time,value in trace:
        if value:rising=time
        elif rising is not None:out.append({"start_ns":rising,"width_ns":time-rising});rising=None
    return out
if __name__=="__main__":
    parser=argparse.ArgumentParser();parser.add_argument("capture",type=Path);parser.add_argument("--output",required=True,type=Path)
    args=parser.parse_args();traces=read_vcd(args.capture)
    data={"sha256":hashlib.sha256(args.capture.read_bytes()).hexdigest(),"channels":list(traces)}
    for name in ("UART_TX","UART_RX"):
        if name in traces:data[name]=uart_lines(uart(traces[name]))
    if "SCL" in traces and "SDA" in traces:
        data["i2c"]=i2c(traces["SCL"],traces["SDA"])
        rising=[t for t,v in traces["SCL"] if v]
        periods=[b-a for a,b in zip(rising,rising[1:]) if 1000<b-a<20000]
        if periods:data["scl_period_median_ns"]=statistics.median(periods)
    for name in ("PIR","ADC_MARK","LED"):
        if name in traces:data[name+"_pulses"]=pulses(traces[name])
    if "ADC_MARK" in traces:
        rising=[t for t,v in traces["ADC_MARK"] if v]
        if len(rising)>1:data["adc_period_median_ns"]=statistics.median(b-a for a,b in zip(rising,rising[1:]))
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.with_suffix(".json").write_text(json.dumps(data,indent=2)+"\n")
    args.output.with_suffix(".uart.txt").write_text("\n".join(f"{x['time_ns']/1e6:.6f} ms {x['text']}" for x in data.get("UART_TX",[]))+"\n")
    print(json.dumps({k:v for k,v in data.items() if not isinstance(v,list)},indent=2))

