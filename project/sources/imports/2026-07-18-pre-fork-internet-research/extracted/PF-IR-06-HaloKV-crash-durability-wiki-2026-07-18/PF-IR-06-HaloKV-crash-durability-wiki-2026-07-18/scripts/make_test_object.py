#!/usr/bin/env python3
"""Create a PF06OBJ1 demonstration object for recovery tests; not a product format."""
from __future__ import annotations
import argparse, hashlib, struct
from pathlib import Path
MAGIC=b'PF06OBJ1';HDR=struct.Struct('>8sIQQ32s')
ap=argparse.ArgumentParser();ap.add_argument('output',type=Path);ap.add_argument('payload');ap.add_argument('--generation',type=int,default=1);a=ap.parse_args()
p=a.payload.encode();a.output.write_bytes(HDR.pack(MAGIC,1,a.generation,len(p),hashlib.sha256(p).digest())+p)
