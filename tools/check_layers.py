#!/usr/bin/env python3
"""Reject hardware dependencies in portable code, including local include chains."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
errors = []
for folder in ("Policy", "Sensors"):
    for f in (root / "firmware" / folder).glob("*.[ch]"):
        text = f.read_text()
        for inc in re.findall(r'^\s*#\s*include\s*[<"]([^>"]+)', text, re.M):
            if any(x in inc.lower() for x in ("stm32", "hal", "cmsis", "board", "main.h", "core/")):
                errors.append(f"{f}: forbidden include {inc}")
        if re.search(r'\b(?:HAL_\w+|GPIO[A-F]|I2C1|USART2|SysTick)\b', text):
            errors.append(f"{f}: hardware symbol")
        for inc in re.findall(r'^\s*#\s*include\s*"([^"]+)', text, re.M):
            resolved = (f.parent / inc).resolve()
            if resolved.exists() and not any((root / "firmware" / x) in resolved.parents for x in ("Policy", "Sensors")):
                errors.append(f"{f}: local include escapes portable layers")
if errors:
    raise SystemExit("\n".join(errors))
print("PASS: Policy and Sensors have no Board/Core/HAL dependencies")

