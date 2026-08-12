# Capture commands

The commands ran from the Windows control PC with these OpenSSH options:

```text
-o BatchMode=yes
-o StrictHostKeyChecking=yes
-o UpdateHostKeys=no
-o ConnectTimeout=10
-o ConnectionAttempts=1
```

The remote command set was read-only:

```bash
date -u +%Y-%m-%dT%H:%M:%SZ
hostname
uname -srmo
systemctl --system show <service> \
  -p ActiveState -p SubState -p MainPID -p InvocationID -p NRestarts \
  -p ExecStart -p FragmentPath
ss -H -ltnp sport = :<port>
```

The coordinator check also used:

```bash
curl -fsS --max-time 5 http://127.0.0.1:8081/health
```

The exact service/port pairs were:

- `nimo-1`: `minimax-m27-q6-server.service`, port `8081`;
- `nimo-2`: `minimax-m27-rpc-worker.service`, port `50052`.
