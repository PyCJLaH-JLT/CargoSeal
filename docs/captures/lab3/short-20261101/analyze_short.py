import sys,json,collections,statistics,hashlib,bisect
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'tools'))
from read_capture import read_vcd,uart,uart_lines
p=Path('short.vcd');t=read_vcd(p);rx=uart_lines(uart(t['RX']));tx=uart_lines(uart(t['TX'])); periods=[]
for (a,v),(b,w) in zip(t['SDA'],t['SDA'][1:]):
 if not v and b-a>100_000_000:periods.append({'start_ns':a,'end_ns':b,'duration_ms':(b-a)/1e6})
print('SDA long low',periods[:20]); print('RX',len(rx),collections.Counter(x['text'] for x in rx));print('TX',len(tx),collections.Counter(x['text'].split(' ')[0] for x in tx));print('first TX',tx[:18]);print('events',[x for x in tx if 'SENSOR_' in x['text']]);print('status samples',[x for x in tx if x['text'].startswith('STATUS ')][:3], [x for x in tx if x['text'].startswith('STATUS ')][-3:]);
result={'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'rx_lines':rx,'tx_lines':tx,'sda_low_intervals_over_100ms':periods};Path('decoded.json').write_text(json.dumps(result,indent=2)+'\n')
# Only isolated STATUS requests after 58 s: associate response before next request.
status=[x for x in rx if x['text']=='STATUS' and x['time_ns']>=58e9]; responses=[x for x in tx if x['text'].startswith('STATUS ')]; pairs=[];missing=[]
for i,x in enumerate(status):
 start=x['time_ns']+len('STATUS\n')*10e9/115200;end=status[i+1]['time_ns'] if i+1<len(status) else 83.75e9;c=[y for y in responses if start<=y['time_ns']<end]
 if len(c)==1:
  y=c[0];complete=y['time_ns']+(len(y['text'])+2)*10e9/115200;pairs.append({'rx_start_ns':x['time_ns'],'response_start_ns':y['time_ns'],'latency_complete_us':(complete-start)/1000})
 else:missing.append({'rx_start_ns':x['time_ns'],'responses':len(c)})
vs=[x['latency_complete_us'] for x in pairs]; print('after58 status',len(status),'paired',len(vs),'ambiguous/missing',len(missing),'latency us',min(vs),statistics.mean(vs),max(vs),'missing',missing);result['status_after_58s']={'request_count':len(status),'pairs':pairs,'unpaired_or_ambiguous':missing,'latency_us':{'min':min(vs),'mean':statistics.mean(vs),'max':max(vs)},'assumption':'FIFO isolated STATUS, no response identity; UART duration estimated nominal 115200 8N1'};Path('decoded.json').write_text(json.dumps(result,indent=2)+'\n')
