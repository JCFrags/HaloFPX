#!/usr/bin/env python3
"""Reference recovery validator for the proposed PF06OBJ1 demonstration format.
This is a test oracle, not the selected HaloKV format implementation.
"""
from __future__ import annotations
import argparse, hashlib, struct, sys
from pathlib import Path
MAGIC=b'PF06OBJ1'
HDR=struct.Struct('>8sIQQ32s')

def main()->int:
    ap=argparse.ArgumentParser();ap.add_argument('file',type=Path);a=ap.parse_args()
    data=a.file.read_bytes()
    if len(data)<HDR.size: print('invalid: short header');return 1
    magic,version,generation,length,digest=HDR.unpack_from(data)
    payload=data[HDR.size:]
    ok=(magic==MAGIC and version==1 and len(payload)==length and hashlib.sha256(payload).digest()==digest)
    print({'valid':ok,'version':version,'generation':generation,'declared_length':length,'actual_length':len(payload),'sha256':hashlib.sha256(payload).hexdigest()})
    return 0 if ok else 1
if __name__=='__main__':sys.exit(main())
