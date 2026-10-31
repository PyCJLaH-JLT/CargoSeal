#!/usr/bin/env python3
"""Build CargoSeal and preserve evidence bound to the actual dirty source tree."""
from pathlib import Path
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from lab3_version import source_version

repo = Path(__file__).resolve().parents[1]
output = repo / 'docs/metrics/lab3-build'
output.mkdir(parents=True, exist_ok=True)
head,digest,hashes=source_version(repo)
build_id = head[:8] + '-' + digest[:12]
(repo / 'firmware/build-id.txt').write_text(build_id + '\n')
prefix = ['docker', 'run', '--rm', '--name', 'cargoseal-lr3-' + digest[:8],
          '-v', str(repo) + ':/workspace', '-w', '/workspace', 'cargoseal-build:lab2']
def run(name, args):
    with (output / name).open('w') as stream:
        result = subprocess.run(prefix + args, stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode:
        raise SystemExit(f'{name}: failed, exit {result.returncode}')
run('build.log', ['make', '-C', 'firmware', '-B', 'all'])
if build_id.encode() not in (repo / 'firmware/build/firmware.elf').read_bytes():
    raise SystemExit('ELF does not contain the requested build ID')
run('stack-audit.log', ['make', '-C', 'firmware', 'stack-audit'])
run('objects-no-lto.log', ['arm-none-eabi-size',*[str(p.relative_to(repo)) for p in sorted((repo/'firmware/build/stack-audit').glob('*.o'))]])
run('disassembly.log', ['arm-none-eabi-objdump', '-d', 'firmware/build/firmware.elf'])
run('sections.log', ['arm-none-eabi-size', '-A', 'firmware/build/firmware.elf'])
run('symbols.log', ['arm-none-eabi-nm', '-S', '--size-sort', '--radix=d', 'firmware/build/firmware.elf'])
run('toolchain.log', ['arm-none-eabi-gcc', '--version'])
for name in ['firmware.elf', 'firmware.hex', 'firmware.bin', 'firmware.map']:
    (output / name).write_bytes((repo / 'firmware/build' / name).read_bytes())
stack_dir = output / 'stack'; stack_dir.mkdir(exist_ok=True)
for path in (repo / 'firmware/build/stack-audit').glob('*.su'):
    (stack_dir / path.name).write_bytes(path.read_bytes())
artifacts = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in output.glob('firmware.*')}
manifest = dict(utc=datetime.now(timezone.utc).isoformat(), head=head, dirty=True,
                build_id=build_id, source_digest=digest, source_sha256=hashes, artifacts=artifacts,
                command=['python3', 'tools/lab3_build.py'], image='cargoseal-build:lab2',
                image_id=subprocess.check_output(['docker','image','inspect','cargoseal-build:lab2','--format','{{.Id}}'],text=True).strip())
manifest['support_sha256']={str(p.relative_to(repo)):hashlib.sha256(p.read_bytes()).hexdigest()
    for p in [repo/'Makefile',repo/'docker/Dockerfile',*sorted((repo/'tools').glob('*.py')),
              *sorted((repo/'tests').glob('*.[ch]')),*sorted((repo/'tests/fakes').glob('*.[ch]'))]}
(output / 'manifest.json').write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + '\n')
print(build_id)
print((output / 'build.log').read_text()[-900:])
