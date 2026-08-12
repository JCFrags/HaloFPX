# Command record

All throughput clients used `iperf3 3.21`, `-t 15 -O 2 -J`. Servers were one-shot (`-s -1 -D`) and bound to the exact peer address.

## Idle latency

```bash
ping -I 10.44.0.1 -c 200 -i 0.02 -q 10.44.0.2
ping -I 10.44.0.5 -c 200 -i 0.02 -q 10.44.0.6
```

## Isolated and concurrent TCP

The rail-specific forms were:

```bash
# peer
iperf3 -s -1 -D -B 10.44.0.2 -p 55201

# initiator, forward
iperf3 -c 10.44.0.2 -B 10.44.0.1 -p 55201 -t 15 -O 2 -J

# initiator, reverse
iperf3 -c 10.44.0.2 -B 10.44.0.1 -p 55201 -t 15 -O 2 -R -J
```

Rail B substituted `10.44.0.5`, `10.44.0.6`, and port `55202`. Concurrent tests ran the two client forms in parallel on ports `55203` and `55204`; loaded pings ran concurrently.

## MPTCP

Both server and client were wrapped by the exact deployed binary `/opt/llm-usb4-cluster/mptcpd/bin/mptcpize` version `0.14`:

```bash
/opt/llm-usb4-cluster/mptcpd/bin/mptcpize run iperf3 -s -1 -D -B 10.44.0.2 -p 55205
/opt/llm-usb4-cluster/mptcpd/bin/mptcpize run iperf3 -c 10.44.0.2 -B 10.44.0.1 -p 55205 -t 15 -O 2 -J
ss -Manie
```

The reverse run added `-R` to the client.

