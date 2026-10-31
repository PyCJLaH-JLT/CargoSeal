#!/usr/bin/env python3
"""Identify the actual CargoSeal firmware sources, including a dirty work tree."""
from pathlib import Path
import hashlib,json,subprocess
def source_version(repo):
    sources=[p for p in (repo/'firmware').rglob('*') if p.is_file()
             and 'build' not in p.relative_to(repo/'firmware').parts and p.name!='build-id.txt']
    sources += [repo/'diagram.json',repo/'wokwi.toml']
    hashes={str(p.relative_to(repo)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(sources)}
    digest=hashlib.sha256(json.dumps(hashes,sort_keys=True).encode()).hexdigest()
    head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()
    return head,digest,hashes
if __name__=='__main__':
    repo=Path(__file__).resolve().parents[1]
    head,digest,_=source_version(repo)
    build_id=head[:8]+'-'+digest[:12]
    (repo/'firmware/build-id.txt').write_text(build_id+'\n')
    print('CargoSeal build',build_id)
