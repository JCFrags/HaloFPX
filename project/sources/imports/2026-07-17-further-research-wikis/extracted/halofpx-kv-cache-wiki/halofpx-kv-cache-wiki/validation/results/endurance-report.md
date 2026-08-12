# SSD endurance scenario report

> Illustrative model. Ratings and WAF are operator-supplied inputs, not drive warranties.

| Scenario | State GiB | Checkpoints/day | Replicas | Host GiB/day | Host TB/year | WAF | Rated TBW | Estimated years |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Light | 4.00 | 10 | 1 | 40.0 | 15.7 | 1.50 | 600 | 25.52 |
| Moderate | 4.00 | 50 | 1 | 200.0 | 78.4 | 1.50 | 1200 | 10.21 |
| Heavy | 4.00 | 200 | 1 | 800.0 | 313.5 | 1.70 | 2400 | 4.50 |
| Long-context heavy | 16.00 | 100 | 1 | 1600.0 | 627.1 | 2.00 | 2400 | 1.91 |
| Target plus 20% optional state | 4.00 | 100 | 1 | 480.0 | 188.1 | 1.60 | 1200 | 3.99 |
| Replicated cache | 4.00 | 50 | 2 | 400.0 | 156.8 | 1.50 | 2400 | 10.21 |

Formula:

```text
years = rated_TBW / (host_TB_per_day × WAF × 365)
```
