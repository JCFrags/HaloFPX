#!/usr/bin/env python3
"""Low-dependency Linux JSONL sampler for host/AMDGPU/network/disk telemetry.

Use AMD SMI/rocprofiler and a wall meter as additional authoritative channels where
available. This sampler records missing fields rather than fabricating zeros.
"""
from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import signal
import time
from pathlib import Path

STOP=False

def stop_handler(*_):
    global STOP; STOP=True

def read_text(path: Path):
    try: return path.read_text(errors='replace').strip()
    except Exception: return None

def read_int(path: Path, scale: float=1.0):
    v=read_text(path)
    try: return int(v)*scale if v is not None else None
    except (ValueError,TypeError): return None

def cpu_ticks():
    line=Path('/proc/stat').read_text().splitlines()[0].split()[1:]
    vals=[int(x) for x in line]; total=sum(vals); idle=vals[3]+(vals[4] if len(vals)>4 else 0)
    return total,idle

def meminfo():
    out={}
    for line in Path('/proc/meminfo').read_text().splitlines():
        k,v=line.split(':',1); parts=v.strip().split(); n=int(parts[0]); out[k.lower()+'_bytes']=n*1024 if len(parts)>1 and parts[1]=='kB' else n
    return out

def psi():
    result={}
    for kind in ['cpu','memory','io']:
        p=Path('/proc/pressure')/kind
        if not p.exists(): continue
        values={}
        for line in p.read_text().splitlines():
            parts=line.split(); cls=parts[0]
            for item in parts[1:]:
                k,v=item.split('=',1); values[f'{cls}_{k}']=float(v) if k!='total' else int(v)
        result[kind]=values
    return result

def net_stats(iface: str|None):
    if not iface: return None
    base=Path('/sys/class/net')/iface/'statistics'
    if not base.exists(): return {'interface':iface,'missing':True}
    keys=['rx_bytes','tx_bytes','rx_packets','tx_packets','rx_errors','tx_errors','rx_dropped','tx_dropped','collisions']
    out={'interface':iface}
    for k in keys: out[k]=read_int(base/k)
    out['operstate']=read_text(Path('/sys/class/net')/iface/'operstate')
    out['mtu']=read_int(Path('/sys/class/net')/iface/'mtu')
    return out

def disk_stats(device: str|None):
    if not device: return None
    for line in Path('/proc/diskstats').read_text().splitlines():
        parts=line.split()
        if len(parts)>=14 and parts[2]==device:
            # sectors are conventionally 512 bytes in /proc/diskstats accounting.
            return {'device':device,'reads_completed':int(parts[3]),'read_bytes':int(parts[5])*512,
                    'read_ms':int(parts[6]),'writes_completed':int(parts[7]),'write_bytes':int(parts[9])*512,
                    'write_ms':int(parts[10]),'io_in_progress':int(parts[11]),'io_ms':int(parts[12]),'weighted_io_ms':int(parts[13])}
    return {'device':device,'missing':True}

def process_stats(pid: int|None):
    if not pid: return None
    base=Path('/proc')/str(pid)
    if not base.exists(): return {'pid':pid,'missing':True}
    out={'pid':pid}
    try:
        for line in (base/'status').read_text().splitlines():
            if line.startswith(('VmRSS:','VmHWM:','Threads:','voluntary_ctxt_switches:','nonvoluntary_ctxt_switches:')):
                k,v=line.split(':',1); parts=v.strip().split(); value=int(parts[0]); out[k.lower()+'_bytes' if parts[1:] and parts[1]=='kB' else k.lower()]=value*1024 if parts[1:] and parts[1]=='kB' else value
        for line in (base/'io').read_text().splitlines():
            k,v=line.split(':',1); out['io_'+k]=int(v.strip())
    except Exception as exc: out['error']=str(exc)
    return out

def discover_gpu(path_arg: str|None):
    if path_arg: return Path(path_arg)
    for p in sorted(Path('/sys/class/drm').glob('card*/device')):
        vendor=read_text(p/'vendor')
        if vendor and vendor.lower()=='0x1002': return p
    return None

def gpu_stats(base: Path|None):
    if not base or not base.exists(): return None
    out={'sysfs_path':str(base),'gpu_busy_percent':read_int(base/'gpu_busy_percent'),'mem_busy_percent':read_int(base/'mem_busy_percent'),
         'vram_total_bytes':read_int(base/'mem_info_vram_total'),'vram_used_bytes':read_int(base/'mem_info_vram_used'),
         'gtt_total_bytes':read_int(base/'mem_info_gtt_total'),'gtt_used_bytes':read_int(base/'mem_info_gtt_used')}
    hwmons=list((base/'hwmon').glob('hwmon*')) if (base/'hwmon').exists() else []
    if hwmons:
        h=hwmons[0]; out['hwmon_name']=read_text(h/'name')
        for f,key,scale in [('temp1_input','temp1_c',0.001),('temp2_input','temp2_c',0.001),('temp3_input','temp3_c',0.001),
                            ('power1_average','power1_w',1e-6),('power1_cap','power1_cap_w',1e-6),('fan1_input','fan1_rpm',1.0)]:
            out[key]=read_int(h/f,scale)
    return out

def thermal_all():
    zones=[]
    for z in sorted(Path('/sys/class/thermal').glob('thermal_zone*')):
        zones.append({'zone':z.name,'type':read_text(z/'type'),'temp_c':read_int(z/'temp',0.001)})
    return {'zones':zones}

def usb4_snapshot():
    devs=[]
    for d in sorted(Path('/sys/bus/thunderbolt/devices').glob('*')):
        if not d.is_dir(): continue
        item={'device':d.name}
        for attr in ['name','device_name','vendor_name','generation','rx_speed','tx_speed','rx_lanes','tx_lanes','authorized','unique_id']:
            v=read_text(d/attr)
            if v is not None: item[attr]=v
        devs.append(item)
    return {'devices':devs} if devs else None

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--run-id',required=True); p.add_argument('--node-id',required=True); p.add_argument('--output',type=Path,required=True)
    p.add_argument('--interval',type=float,default=1.0); p.add_argument('--duration',type=float,default=0.0)
    p.add_argument('--interface'); p.add_argument('--block-device'); p.add_argument('--pid',type=int); p.add_argument('--gpu-sysfs')
    p.add_argument('--usb4-every',type=int,default=60,help='Sample USB4 topology every N samples; 0 disables')
    a=p.parse_args(); a.output.parent.mkdir(parents=True,exist_ok=True)
    signal.signal(signal.SIGINT,stop_handler); signal.signal(signal.SIGTERM,stop_handler)
    gpu=discover_gpu(a.gpu_sysfs); start=time.monotonic(); prev_total,prev_idle=cpu_ticks(); sample_no=0
    with a.output.open('a',encoding='utf-8',buffering=1) as fh:
        while not STOP and (a.duration<=0 or time.monotonic()-start<a.duration):
            loop_start=time.monotonic(); now_ns=time.monotonic_ns(); total,idle=cpu_ticks(); dticks=total-prev_total; didle=idle-prev_idle
            util=(1-didle/dticks)*100 if dticks>0 else None; prev_total,prev_idle=total,idle
            err=None
            try:
                record={'schema_version':1,'run_id':a.run_id,'node_id':a.node_id,'sample_monotonic_ns':now_ns,
                        'sample_utc':dt.datetime.now(dt.timezone.utc).isoformat().replace('+00:00','Z'),'sample_interval_ms':round(a.interval*1000),
                        'source':'reference_linux_sampler/1','cpu':{'total_util_percent':util,'loadavg':os.getloadavg(),'process':process_stats(a.pid)},
                        'memory':meminfo(),'gpu':gpu_stats(gpu),'disk':disk_stats(a.block_device),'network':net_stats(a.interface),
                        'usb4':usb4_snapshot() if a.usb4_every and sample_no%a.usb4_every==0 else None,'power':None,
                        'thermal':thermal_all(),'psi':psi(),'runtime':None,'collector_error':err}
            except Exception as exc:
                record={'schema_version':1,'run_id':a.run_id,'node_id':a.node_id,'sample_monotonic_ns':now_ns,
                        'sample_utc':dt.datetime.now(dt.timezone.utc).isoformat().replace('+00:00','Z'),'sample_interval_ms':round(a.interval*1000),
                        'source':'reference_linux_sampler/1','cpu':None,'memory':None,'gpu':None,'disk':None,'network':None,'usb4':None,
                        'power':None,'thermal':None,'psi':None,'runtime':None,'collector_error':str(exc)}
            fh.write(json.dumps(record,separators=(',',':'))+'\n'); sample_no+=1
            delay=a.interval-(time.monotonic()-loop_start)
            if delay>0: time.sleep(delay)
    print(json.dumps({'samples':sample_no,'output':str(a.output),'gpu_sysfs':str(gpu) if gpu else None}))
    return 0
if __name__=='__main__': raise SystemExit(main())
