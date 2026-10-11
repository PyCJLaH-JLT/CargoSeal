#!/usr/bin/env python3
"""Measure the committed real acceptance recording, using wire timestamps."""
import json, math, statistics
from pathlib import Path
from read_capture import read_vcd, uart, uart_lines, i2c, pulses

root=Path(__file__).resolve().parents[1]
traces=read_vcd(root/"docs/captures/final-acceptance.vcd.gz")
tx=uart_lines(uart(traces["UART_TX"]))
received=uart(traces["UART_RX"])
commands=[];buffer=bytearray()
for byte in received:
    if byte["byte"]==10:
        commands.append((byte["time_ns"]+10e9/115200,buffer.decode().rstrip("\r")))
        buffer.clear()
    else:buffer.append(byte["byte"])
responses=[]
for ended,command in commands:
    prefix="STATUS " if command=="STATUS" else "ERR COMMAND" if command=="BAD" else "OK MODE " if command.startswith("MODE ") else None
    if prefix:
        response=next(x for x in tx if x["time_ns"]>=ended and x["text"].startswith(prefix))
        responses.append((response["time_ns"]+(len(response["text"])+2)*10e9/115200-ended)/1e6)
events=[x for x in tx if x["text"].startswith("EVENT ")]
assert [int(x["text"].split("seq=")[1].split()[0]) for x in events]==list(range(1,37))
assert max(responses)<100
assert all("tx_dropped=0 rx_dropped=0" in x["text"] for x in tx if x["text"].startswith("STATUS "))
bus=i2c(traces["SCL"],traces["SDA"]);segment=[];frames=[]
for item in bus:
    if item["kind"]=="START":segment=[item]
    elif item["kind"]=="STOP":
        bytes_=[x for x in segment if x["kind"]=="BYTE"]
        if len(bytes_)==15 and bytes_[0]["hex"]=="D1":
            frames.append((segment[0]["time_ns"],item["time_ns"],bytes_))
        segment=[]
    else:segment.append(item)
assert any(x.get("hex")=="D2" and not x["ack"] for x in bus)
first=frames[0][2];raw=[int(x["hex"],16) for x in first[1:]]
assert raw==[0,0,0,0,16,0,239,92,0,0,0,0,0,0]
adc=pulses(traces["ADC_MARK"]);periods=[(b["start_ns"]-a["start_ns"])/1e6 for a,b in zip(adc,adc[1:])]
result={
 "events":len(events),"complete_response_after_lf_ms":{"min":min(responses),"max":max(responses),"count":len(responses)},
 "imu_period_median_ms":statistics.median((b[0]-a[0])/1e6 for a,b in zip(frames,frames[1:])),
 "adc_period_ms":{"min":min(periods),"median":statistics.median(periods),"max":max(periods)},
 "adc_mark_width_median_us":statistics.median(x["width_ns"] for x in adc)/1000,
 "pir_high_ms":[x["width_ns"]/1e6 for x in pulses(traces["PIR"])],
 "first_burst_raw_hex":" ".join(f"{x:02X}" for x in raw),
 "read_phase_duration_us":(frames[0][1]-frames[0][0])/1000,
 "note":"Response completion includes CRLF and the stop bit of LF. GPIO marker spans the entire HAL ADC call. UI pausing stops simulated time."
}
(root/"docs/captures/measurements.json").write_text(json.dumps(result,indent=2)+"\n")
print(json.dumps(result,indent=2))
