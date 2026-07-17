# API examples

All examples are available as executable or directly consumable files in [`examples/`](examples). The request user fields shown below are **routing/cache hints, not authentication credentials**. A production port must derive tenant ownership from authenticated middleware.

## OpenAI-compatible chat with a cache/user hint

```bash
curl --fail-with-body -sS   -H "Authorization: Bearer ${API_KEY}"   -H "Content-Type: application/json"   http://127.0.0.1:9090/v1/chat/completions   -d '{
    "model": "configured-model-name",
    "messages": [
      {"role": "system", "content": "Answer with compact, auditable reasoning."},
      {"role": "user", "content": "Summarize the cache design."}
    ],
    "cache_prompt": true,
    "llama_user_id": "tenant42-user7",
    "stream": false
  }'
```

Source evidence: [E-081](20-Evidence-Index.md#e-081), [E-087](20-Evidence-Index.md#e-087), [E-102](20-Evidence-Index.md#e-102).

## Anthropic Messages user hint

```bash
curl --fail-with-body -sS   -H "Authorization: Bearer ${API_KEY}"   -H "anthropic-version: 2023-06-01"   -H "Content-Type: application/json"   http://127.0.0.1:9090/v1/messages   -d '{
    "model": "configured-model-name",
    "max_tokens": 128,
    "system": "Answer with compact, auditable reasoning.",
    "messages": [{"role": "user", "content": "Summarize the cache design."}],
    "metadata": {"user_id": "tenant42-user7"}
  }'
```

## Slot save, restore, and erase

Slot actions require `--slot-save-path`.

```bash
curl -sS -X POST   -H "Content-Type: application/json"   'http://127.0.0.1:9090/slots/0?action=save'   -d '{"filename":"review-session.bin"}'

curl -sS -X POST   -H "Content-Type: application/json"   'http://127.0.0.1:9090/slots/0?action=restore'   -d '{"filename":"review-session.bin"}'

curl -sS -X POST   'http://127.0.0.1:9090/slots/0?action=erase'
```

Evidence: [E-091](20-Evidence-Index.md#e-091).

## Router model lifecycle

```bash
curl -sS 'http://127.0.0.1:9090/models?reload=1'

curl -sS -X POST   -H 'Content-Type: application/json'   http://127.0.0.1:9090/models/load   -d '{"model":"configured-model-name"}'

curl -sS -X POST   -H 'Content-Type: application/json'   http://127.0.0.1:9090/models/unload   -d '{"model":"configured-model-name"}'

curl -N http://127.0.0.1:9090/models/sse
```

Evidence: [E-095](20-Evidence-Index.md#e-095), [E-106](20-Evidence-Index.md#e-106).

## Resumable stream session

```bash
curl -N "http://127.0.0.1:9090/v1/stream/${CONVERSATION_ID}"

curl -sS -X POST   -H 'Content-Type: application/json'   http://127.0.0.1:9090/v1/streams/lookup   -d '{"conversation_ids":["conv-a","conv-b"]}'

curl -sS -X DELETE   "http://127.0.0.1:9090/v1/stream/${CONVERSATION_ID}"
```

Evidence: [E-097](20-Evidence-Index.md#e-097)–[E-099](20-Evidence-Index.md#e-099).

## Observability

```bash
curl -sS http://127.0.0.1:9090/health
curl -sS http://127.0.0.1:9090/props
curl -sS http://127.0.0.1:9090/slots
curl -sS http://127.0.0.1:9090/metrics
curl -sS http://127.0.0.1:9090/expert-stats
```

`/metrics` requires `--metrics`; `/slots` can be controlled with `--slots`; expert tracking is an optional diagnostic surface.

## Verified CachyLlama cache flags used by the parent

```bash
llama-server   --model /models/model.gguf   --cache-ssd /var/lib/cachyllama/cache   --cache-ssd-checkpoints 64   --cache-ssd-hot-window 4096   --cache-ssd-warm-window 32768   --cache-ssd-max-cold 32   --cache-ssd-page-size 1024   --cache-ssd-hot-ram 6144   --cache-ssd-warm-ram 2048   --cache-ssd-system-prompts 8   --cache-ssd-system-max-days 30
```

Use `--cache-ssd-no-fsync` only when explicitly accepting weaker durability. Exact command assembly is evidenced by [E-006](20-Evidence-Index.md#e-006).

## ROCmFPX current run-scoped disk cache

```bash
llama-server   --model /models/model.gguf   --cache-ram 8192   --cache-disk /var/cache/rocmfpx/run-cache   --cache-disk-limit 8192   --metrics
```

This directory is process-owned and removed on clean shutdown at the assessed target pin. Evidence: [E-203](20-Evidence-Index.md#e-203)–[E-209](20-Evidence-Index.md#e-209).

## Included files

- [`01-openai-chat-user-cache.sh`](examples/01-openai-chat-user-cache.sh)
- [`02-anthropic-user-cache.sh`](examples/02-anthropic-user-cache.sh)
- [`03-slot-save-restore.sh`](examples/03-slot-save-restore.sh)
- [`04-model-lifecycle.sh`](examples/04-model-lifecycle.sh)
- [`05-observability.sh`](examples/05-observability.sh)
- [`06-stream-resume.sh`](examples/06-stream-resume.sh)
- [`07-openai-python.py`](examples/07-openai-python.py)
- [`08-cachyllama-cache-flags.sh`](examples/08-cachyllama-cache-flags.sh)
- [`09-rocmfpx-current-run-cache.sh`](examples/09-rocmfpx-current-run-cache.sh)
- [`10-rocmfpx-proposed-persistent-config.json`](examples/10-rocmfpx-proposed-persistent-config.json)
