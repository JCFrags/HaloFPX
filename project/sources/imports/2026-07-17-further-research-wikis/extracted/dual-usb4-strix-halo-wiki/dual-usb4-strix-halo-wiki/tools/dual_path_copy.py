#!/usr/bin/env python3
"""Integrity-checked two-socket file striper for dual USB4NET research.

This is deliberately small and auditable. It has no authentication or encryption
and is not a production RPC protocol.
"""
from __future__ import annotations
import argparse, hashlib, os, queue, socket, struct, threading, time
from dataclasses import dataclass
from pathlib import Path

MAGIC = b"DUSB4DP1"
HELLO, DATA, DONE = 1, 2, 3
HEADER = struct.Struct("!8sBQQQI32s")  # magic,type,session,offset,total,length,hash


def parse_endpoint(text: str) -> tuple[str, int]:
    host, port = text.rsplit(":", 1); return host, int(port)

def recv_exact(s: socket.socket, n: int) -> bytes:
    out = bytearray()
    while len(out) < n:
        data = s.recv(n - len(out))
        if not data: raise EOFError("peer closed")
        out.extend(data)
    return bytes(out)

def pwrite_all(fd: int, data: bytes, offset: int) -> None:
    view = memoryview(data)
    written = 0
    while view:
        n = os.pwrite(fd, view, offset + written)
        if n <= 0:
            raise OSError("pwrite made no progress")
        written += n
        view = view[n:]


def file_hash(path: Path) -> bytes:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while data := f.read(4 << 20): h.update(data)
    return h.digest()

@dataclass
class ServerState:
    output: Path
    expected_total: int | None = None
    expected_hash: bytes | None = None
    session: int | None = None
    ranges: list[tuple[int, int]] = None  # type: ignore
    lock: threading.Lock = None  # type: ignore
    errors: list[str] = None  # type: ignore
    done: int = 0
    def __post_init__(self) -> None:
        self.ranges, self.lock, self.errors = [], threading.Lock(), []

def serve_path(bind: tuple[str,int], state: ServerState, path_id: int) -> None:
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as ls:
            ls.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1); ls.bind(bind); ls.listen(1)
            print(f"path{path_id} listening on {bind[0]}:{bind[1]}", flush=True)
            conn, peer = ls.accept()
            with conn:
                print(f"path{path_id} accepted {peer}", flush=True)
                while True:
                    raw = recv_exact(conn, HEADER.size)
                    magic, typ, session, offset, total, length, digest = HEADER.unpack(raw)
                    if magic != MAGIC: raise ValueError("bad magic")
                    if typ == HELLO:
                        with state.lock:
                            if state.session not in (None, session): raise ValueError("session mismatch")
                            if state.expected_total not in (None, total): raise ValueError("total mismatch")
                            if state.expected_hash not in (None, digest): raise ValueError("file hash mismatch between paths")
                            state.session, state.expected_total, state.expected_hash = session, total, digest
                    elif typ == DATA:
                        data = recv_exact(conn, length)
                        if hashlib.sha256(data).digest() != digest: raise ValueError(f"chunk hash mismatch at {offset}")
                        if offset + length > total: raise ValueError("chunk exceeds total length")
                        fd = os.open(state.output, os.O_RDWR | os.O_CREAT, 0o600)
                        try:
                            pwrite_all(fd, data, offset)
                        finally:
                            os.close(fd)
                        with state.lock: state.ranges.append((offset, offset + length))
                    elif typ == DONE:
                        with state.lock: state.done += 1
                        return
                    else: raise ValueError(f"unknown frame type {typ}")
    except Exception as exc:
        with state.lock: state.errors.append(f"path{path_id}: {exc}")

def run_server(args: argparse.Namespace) -> None:
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # Remove stale content so interrupted prior runs cannot be mistaken for data
    # received in this session. The final coverage map and SHA-256 remain decisive.
    with args.output.open("wb"):
        pass
    state = ServerState(args.output)
    threads = [threading.Thread(target=serve_path, args=(parse_endpoint(ep), state, i), daemon=True) for i, ep in enumerate((args.bind0,args.bind1))]
    for t in threads: t.start()
    for t in threads: t.join()
    if state.errors: raise SystemExit("; ".join(state.errors))
    if state.done != 2 or state.expected_total is None or state.expected_hash is None: raise SystemExit("incomplete session")
    merged=[]
    for a,b in sorted(state.ranges):
        if not merged or a > merged[-1][1]: merged.append([a,b])
        elif a < merged[-1][1]: raise SystemExit(f"overlap detected at {a}")
        else: merged[-1][1]=b
    expected_ranges = [] if state.expected_total == 0 else [[0, state.expected_total]]
    if merged != expected_ranges:
        raise SystemExit(f"range coverage incomplete: {merged[:8]}")
    os.truncate(state.output, state.expected_total)
    actual=file_hash(state.output)
    if actual != state.expected_hash: raise SystemExit(f"final hash mismatch expected={state.expected_hash.hex()} actual={actual.hex()}")
    print(f"VERIFIED {state.expected_total} bytes sha256={actual.hex()}")

def connect_path(spec: str) -> socket.socket:
    # source,peer:port
    source, target = spec.split(",", 1); peer=parse_endpoint(target)
    s=socket.socket(socket.AF_INET,socket.SOCK_STREAM); s.bind((source,0)); s.connect(peer); return s

def send_worker(s: socket.socket, q: queue.Queue, session: int, total: int, digest: bytes, stats: dict, key: str) -> None:
    s.sendall(HEADER.pack(MAGIC,HELLO,session,0,total,0,digest))
    sent=0; started=time.monotonic()
    while True:
        item=q.get()
        if item is None: break
        offset,data=item; ch=hashlib.sha256(data).digest()
        s.sendall(HEADER.pack(MAGIC,DATA,session,offset,total,len(data),ch)); s.sendall(data); sent+=len(data)
    s.sendall(HEADER.pack(MAGIC,DONE,session,0,total,0,digest)); s.shutdown(socket.SHUT_WR); s.close()
    stats[key]=(sent,time.monotonic()-started)

def run_client(args: argparse.Namespace) -> None:
    if args.chunk <= 0:
        raise SystemExit("--chunk must be positive")
    if args.queue_depth <= 0:
        raise SystemExit("--queue-depth must be positive")
    if not args.input.is_file():
        raise SystemExit(f"input is not a regular file: {args.input}")
    total=args.input.stat().st_size; digest=file_hash(args.input); session=int.from_bytes(os.urandom(8),"big")
    sockets=[connect_path(args.path0),connect_path(args.path1)]; qs=[queue.Queue(args.queue_depth),queue.Queue(args.queue_depth)]; stats={}
    threads=[threading.Thread(target=send_worker,args=(sockets[i],qs[i],session,total,digest,stats,f"path{i}")) for i in range(2)]
    for t in threads:t.start()
    with args.input.open("rb") as f:
        offset=0; i=0
        while data:=f.read(args.chunk): qs[i%2].put((offset,data)); offset+=len(data); i+=1
    for q in qs:q.put(None)
    for t in threads:t.join()
    for k,(n,sec) in stats.items(): print(f"{k}: {n} bytes {n*8/sec/1e9:.3f} Gbit/s")
    print(f"sent {total} bytes sha256={digest.hex()}")

def main() -> None:
    p=argparse.ArgumentParser(description=__doc__); sub=p.add_subparsers(dest="mode",required=True)
    s=sub.add_parser("server"); s.add_argument("--bind0",default="0.0.0.0:6100"); s.add_argument("--bind1",default="0.0.0.0:6101"); s.add_argument("--output",type=Path,required=True)
    c=sub.add_parser("client"); c.add_argument("--path0",required=True,help="LOCAL_IP,PEER_IP:PORT"); c.add_argument("--path1",required=True); c.add_argument("--input",type=Path,required=True); c.add_argument("--chunk",type=int,default=1<<20); c.add_argument("--queue-depth",type=int,default=8)
    a=p.parse_args(); run_server(a) if a.mode=="server" else run_client(a)
if __name__=="__main__": main()
