#!/usr/bin/env python3
"""Summarize the saved UART TX and FLASH_MARK signals from a Wokwi VCD."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path

from read_capture import uart, uart_lines


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('capture', type=Path, help='original .vcd or .vcd.gz')
    args = parser.parse_args()
    opener = gzip.open if args.capture.suffix == '.gz' else open
    tx, marker, digest, now = [], [], hashlib.sha256(), 0
    with opener(args.capture, 'rb') as data:
        for raw in data:
            digest.update(raw)
            line = raw.decode('ascii').strip()
            if line.startswith('#'):
                now = int(line[1:])
            elif line in ('0#', '1#'):
                tx.append((now, int(line[0])))
            elif line in ("0'", "1'"):
                marker.append((now, int(line[0])))
    result = {'uncompressed_sha256': digest.hexdigest(), 'vcd_end_ns': now,
              'flash_marker_transitions': marker, 'uart_tx_lines': uart_lines(uart(tx))}
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
