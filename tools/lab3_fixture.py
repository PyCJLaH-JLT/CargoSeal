#!/usr/bin/env python3
"""Produce declared stimulus diagrams, never captures or substitute results."""
import copy,json
from pathlib import Path
repo=Path(__file__).resolve().parents[1]
base=json.loads((repo/'diagram.json').read_text())
out=repo/'tools/scenarios';out.mkdir(exist_ok=True)
for name,frequency,uart_rx,loop in [('S1',None,False,False),('S2','0.008333333333',False,False),('S3','5',False,False),('uart','5',True,False),('loop',None,False,True)]:
 d=copy.deepcopy(base)
 if frequency:
  d['connections']=[x for x in d['connections'] if x[:2]!=['pir:OUT','nucleo:PA1']]
  d['parts'].append(dict(type='wokwi-clock-generator',id='cargoStimulus',top=220,left=580,attrs={'frequency':frequency}))
  d['connections'].append(['cargoStimulus:CLK','nucleo:PA1','orange',[]])
 if not loop:d['connections']=[x for x in d['connections'] if x[0]!='logic:D7']
 if uart_rx:
  d['connections']=[x for x in d['connections'] if x[0]!='logic:D0']
  d['connections'].append(['logic:D0','nucleo:PA3','gray',[]])
  next(x for x in d['parts'] if x['id']=='logic')['attrs']['channelNames']='UART_RX,SDA,UART_TX,PIR,ADC_MARK,IMU_MARK,FLASH_MARK,LOOP_MARK'
 folder=out/name;folder.mkdir(exist_ok=True)
 (folder/'diagram.json').write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n')
 (folder/'wokwi.toml').write_text("[wokwi]\nversion=1\nfirmware='../../../firmware/build/firmware.hex'\nelf='../../../firmware/build/firmware.elf'\nrfc2217ServerPort=43106\nvcdFile='current-run.vcd'\n")
 print(name,'digital fixture frequency',frequency,'Hz; PIR OUT disconnected' if frequency else 'original PIR sensor')
