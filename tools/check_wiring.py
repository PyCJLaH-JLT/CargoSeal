#!/usr/bin/env python3
from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
data=json.loads((root/"diagram.json").read_text())
edges={frozenset(c[:2]) for c in data["connections"]}
required=[("imu:SCL","nucleo:PB8"),("imu:SDA","nucleo:PB9"),("pir:OUT","nucleo:PA1"),
          ("light:AO","nucleo:PA0"),("logic:D4","nucleo:PA4"),("$serialMonitor:TX","nucleo:PA3"),
          ("$serialMonitor:RX","nucleo:PA2")]
assert all(frozenset(e) in edges for e in required),"pinout mismatch"
ioc=(root/"firmware/firmware.ioc").read_text()
for setting in ("PA0.Signal=ADC1_IN0","PA1.Signal=GPIO_Input","PA4.Signal=GPIO_Output",
                "PB8.Signal=I2C1_SCL","PB9.Signal=I2C1_SDA","ADC1.Resolution=ADC_RESOLUTION_12B"):
    assert setting in ioc,setting
assert len({p["id"] for p in data["parts"]})==len(data["parts"])
print("PASS: CargoSeal sensor/monitor pins match .ioc and diagram")

