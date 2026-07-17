#!/usr/bin/env python3
"""Timestamp OpenAI-compatible streaming responses into request/token JSONL.

This is a reference acquisition client, not a high-load generator. For true ITL,
confirm that each recorded content event corresponds to one model token or enrich
records with server token IDs.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import datetime as dt
import hashlib
import json
import os
import sys
import time
import urllib.error
import urllib.request
import uuid
from pathlib import Path
from threading import Lock


def utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat().replace("+00:00", "Z")


def sha(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def load_prompts(path: Path) -> list[dict]:
    rows=[]
    for n,line in enumerate(path.read_text(encoding='utf-8').splitlines(),1):
        if not line.strip(): continue
        row=json.loads(line)
        for key in ['prompt_id','prompt','prompt_tokens']:
            if key not in row: raise ValueError(f'{path}:{n} missing {key}')
        rows.append(row)
    if not rows: raise ValueError('prompt file is empty')
    return rows


def extract_content(obj: dict, mode: str) -> tuple[str, str | None]:
    choices=obj.get('choices') or []
    if not choices: return '', None
    choice=choices[0] or {}
    finish=choice.get('finish_reason')
    if mode=='chat':
        delta=choice.get('delta') or {}
        content=delta.get('content')
        if isinstance(content,list):
            content=''.join(x.get('text','') if isinstance(x,dict) else str(x) for x in content)
        return content or '', finish
    return choice.get('text') or '', finish


def run_one(row: dict, args, headers: dict[str,str]) -> tuple[dict,list[dict],str|None]:
    rid=f"{args.run_id}-{row['prompt_id']}-{uuid.uuid4().hex[:10]}"
    body=dict(args.template)
    body.update({'model':args.model,'stream':True,'temperature':args.temperature,'seed':args.seed,
                 'max_tokens':int(row.get('max_tokens',args.max_tokens))})
    if args.mode=='chat': body['messages']=row.get('messages') or [{'role':'user','content':row['prompt']}]
    else: body['prompt']=row['prompt']
    body.setdefault('stream_options',{'include_usage':True})
    send_ns=time.monotonic_ns(); send_utc=utc_now()
    first_ns=last_ns=None; finish_reason=None; http_status=None; error_class=None; error_message=None
    content_parts=[]; events=[]; usage={}; timings={}; success=False
    try:
        req=urllib.request.Request(args.endpoint,data=json.dumps(body).encode(),headers=headers,method='POST')
        with urllib.request.urlopen(req,timeout=args.timeout) as resp:
            http_status=getattr(resp,'status',200)
            for raw in resp:
                line=raw.decode('utf-8','replace').strip()
                if not line or line.startswith(':'): continue
                payload=line[5:].strip() if line.startswith('data:') else line
                if payload=='[DONE]': break
                try: obj=json.loads(payload)
                except json.JSONDecodeError: continue
                if isinstance(obj.get('usage'),dict): usage.update(obj['usage'])
                if isinstance(obj.get('timings'),dict): timings.update(obj['timings'])
                text,finish=extract_content(obj,args.mode)
                if finish is not None: finish_reason=finish
                if text:
                    now=time.monotonic_ns(); now_utc=utc_now()
                    if first_ns is None: first_ns=now
                    last_ns=now; content_parts.append(text)
                    events.append({'schema_version':1,'run_id':args.run_id,'request_id':rid,'sequence':len(events),
                                   'client_event_monotonic_ns':now,'client_event_utc':now_utc,'server_event_monotonic_ns':None,
                                   'token_id':None,'token_text_sha256':sha(text),'content_bytes':len(text.encode()),
                                   'is_content_token':True,'is_terminal_event':False,'raw_event_bytes':len(raw),
                                   'event_token_count':None,'event_count_is_token_count':False})
            success=200 <= int(http_status) < 300
    except urllib.error.HTTPError as exc:
        http_status=exc.code; error_class='HTTPError'; error_message=str(exc)
    except Exception as exc:
        error_class=type(exc).__name__; error_message=str(exc)
    complete_ns=time.monotonic_ns(); complete_utc=utc_now(); output=''.join(content_parts)

    prompt_tokens=int(row['prompt_tokens'])
    output_tokens=None; method='unknown'; estimated=False
    if usage.get('completion_tokens') is not None:
        output_tokens=int(usage['completion_tokens']); method='server_usage'
    elif timings.get('predicted_n') is not None:
        output_tokens=int(timings['predicted_n']); method='server_timings'
    elif args.allow_event_count_estimate:
        output_tokens=len(events); method='event_estimate'; estimated=True
    if output_tokens is None and not args.allow_missing_token_count:
        success=False
        error_class=error_class or 'MissingTokenCount'
        error_message=error_message or 'Server did not report completion token count; rerun with usage/timings or allow missing.'

    cached=None; processed=None
    for source in (usage,timings):
        if source.get('cached_prompt_tokens') is not None: cached=int(source['cached_prompt_tokens'])
        if source.get('prompt_tokens_cached') is not None: cached=int(source['prompt_tokens_cached'])
        if source.get('prompt_n') is not None and processed is None: processed=int(source['prompt_n'])
        if source.get('prompt_tokens') is not None and processed is None: processed=int(source['prompt_tokens'])

    record={'schema_version':1,'run_id':args.run_id,'request_id':rid,'prompt_id':row['prompt_id'],'topology':args.topology,
            'cache_state':args.cache_state,'client_send_monotonic_ns':send_ns,'client_first_token_monotonic_ns':first_ns,
            'client_last_token_monotonic_ns':last_ns,'client_complete_monotonic_ns':complete_ns,'client_send_utc':send_utc,
            'client_complete_utc':complete_utc,'server_received_monotonic_ns':None,'server_first_token_monotonic_ns':None,
            'prompt_tokens':prompt_tokens,'eligible_prefix_tokens':row.get('eligible_prefix_tokens'),
            'cached_prompt_tokens':cached,'processed_prompt_tokens':processed,'output_tokens':output_tokens,
            'max_output_tokens':int(row.get('max_tokens',args.max_tokens)),'http_status':http_status,'success':bool(success),
            'finish_reason':finish_reason,'error_class':error_class,'error_message_redacted':error_message,
            'output_sha256':sha(output) if output else None,'correctness_record_id':None,
            'server_timings':{'usage':usage,'timings':timings} if usage or timings else None,'queue_depth_at_submit':None,
            'attempt':1,'token_count_method':method,'output_tokens_estimated':estimated,'stream_event_count':len(events)}
    return record,events,output if args.retain_output else None


def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--endpoint',required=True); p.add_argument('--model',required=True); p.add_argument('--prompts',type=Path,required=True)
    p.add_argument('--run-id',required=True); p.add_argument('--topology',choices=['node-a','node-b','dual'],required=True)
    p.add_argument('--cache-state',choices=['C0','C1','C2','C3','NA'],default='C2'); p.add_argument('--mode',choices=['chat','completions'],default='chat')
    p.add_argument('--max-tokens',type=int,default=128); p.add_argument('--temperature',type=float,default=0); p.add_argument('--seed',type=int,default=1)
    p.add_argument('--concurrency',type=int,default=1); p.add_argument('--timeout',type=float,default=600)
    p.add_argument('--api-key-env',default='OPENAI_API_KEY'); p.add_argument('--template-json',type=Path)
    p.add_argument('--requests-out',type=Path,required=True); p.add_argument('--tokens-out',type=Path,required=True); p.add_argument('--outputs-out',type=Path)
    p.add_argument('--allow-event-count-estimate',action='store_true'); p.add_argument('--allow-missing-token-count',action='store_true')
    a=p.parse_args(); a.template=json.loads(a.template_json.read_text()) if a.template_json else {}; a.retain_output=bool(a.outputs_out)
    prompts=load_prompts(a.prompts); key=os.environ.get(a.api_key_env)
    headers={'Content-Type':'application/json','Accept':'text/event-stream'}
    if key: headers['Authorization']=f'Bearer {key}'
    results=[]
    with concurrent.futures.ThreadPoolExecutor(max_workers=a.concurrency) as pool:
        futures=[pool.submit(run_one,row,a,headers) for row in prompts]
        for f in concurrent.futures.as_completed(futures): results.append(f.result())
    results.sort(key=lambda x:x[0]['client_send_monotonic_ns'])
    a.requests_out.parent.mkdir(parents=True,exist_ok=True); a.tokens_out.parent.mkdir(parents=True,exist_ok=True)
    a.requests_out.write_text(''.join(json.dumps(r,separators=(',',':'))+'\n' for r,_,_ in results))
    a.tokens_out.write_text(''.join(json.dumps(t,separators=(',',':'))+'\n' for _,ts,_ in results for t in ts))
    if a.outputs_out:
        a.outputs_out.write_text(''.join(json.dumps({'request_id':r['request_id'],'output':o})+'\n' for r,_,o in results))
    failed=sum(not r['success'] for r,_,_ in results)
    print(json.dumps({'requests':len(results),'failed_or_incomplete':failed,'requests_out':str(a.requests_out),'tokens_out':str(a.tokens_out)}))
    return 1 if failed else 0
if __name__=='__main__': sys.exit(main())
