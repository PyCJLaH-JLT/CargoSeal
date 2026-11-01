import serial,time,json,pathlib,os
p=pathlib.Path(__file__).resolve().parent
url='rfc2217://127.0.0.1:50841'
s=serial.serial_for_url(url,baudrate=115200,timeout=0.1)
start=time.monotonic()
with open(p/'uart.raw','wb',buffering=0) as raw, open(p/'commands.jsonl','w',buffering=1) as log:
 def drain(secs):
  end=time.monotonic()+secs
  while time.monotonic()<end: raw.write(s.read(4096))
 def command(x):
  s.write((x+'\r\n').encode());log.write(json.dumps({'host_monotonic':time.monotonic(),'command':x})+'\n')
 drain(1);command('STATUS');drain(2);command('LOG STATUS');drain(2)
 print('INITIAL_STATUS_READY',flush=True)
 while not (p/'armed').exists():
  drain(.2)
  if time.monotonic()-start>120: raise TimeoutError('debugger did not arm, ERASE not sent')
 if not (p/'nvm-before.bin').exists() or (p/'nvm-before.bin').stat().st_size!=4096: raise RuntimeError('preimage missing, ERASE not sent')
 command('LOG ERASE');drain(10);command('LOG STATUS');drain(3)
print('DONE',flush=True)
s.close()
