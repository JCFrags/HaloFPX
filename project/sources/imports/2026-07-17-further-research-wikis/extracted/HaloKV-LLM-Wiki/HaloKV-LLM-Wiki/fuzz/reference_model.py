#!/usr/bin/env python3
"""Small HaloKV protocol oracle for stateful traces.

Input is a JSON array of events. This model intentionally abstracts bytes,
authority storage, and transport framing; it checks core state invariants.
"""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

RANKS = {0, 1}


@dataclass
class Model:
    epoch: int = 0
    phase: str = "IDLE"
    durable: set[int] = field(default_factory=set)
    prepared: set[int] = field(default_factory=set)
    cancelled: bool = False
    topology_ok: dict[int, bool] = field(default_factory=lambda: {0: True, 1: True})
    corrupt: dict[int, bool] = field(default_factory=lambda: {0: False, 1: False})
    accepted_epoch: dict[int, int] = field(default_factory=lambda: {0: 0, 1: 0})
    last_read: str = "NONE"

    def apply(self, event: dict[str, Any]) -> str:
        kind = event.get("kind")
        if kind == "bump_epoch":
            new_epoch = int(event["epoch"])
            if new_epoch <= self.epoch:
                return "REJECT_NON_MONOTONIC_EPOCH"
            self.epoch = new_epoch
            self.phase = "IDLE"
            self.durable.clear()
            self.prepared.clear()
            self.cancelled = False
            self.last_read = "NONE"
            result = "OK"
        elif kind == "accept_epoch":
            rank = self._rank(event)
            if int(event["epoch"]) != self.epoch:
                result = "REJECT_UNPROVEN_OR_STALE_EPOCH"
            else:
                self.accepted_epoch[rank] = self.epoch
                result = "OK"
        elif kind == "begin":
            if self.phase != "IDLE":
                result = "REJECT_BAD_STATE"
            elif int(event.get("epoch", self.epoch)) != self.epoch:
                result = "REJECT_STALE_EPOCH"
            else:
                self.phase = "OPEN"
                result = "OK"
        elif kind == "durable":
            rank = self._rank(event)
            if not self._current_rank_can_mutate(rank) or self.phase != "OPEN":
                result = "REJECT_PRECONDITION"
            else:
                self.durable.add(rank)
                result = "OK"
        elif kind == "prepare":
            rank = self._rank(event)
            msg_epoch = int(event.get("epoch", self.epoch))
            if self.phase != "OPEN" or self.cancelled:
                result = "REJECT_TERMINAL_OR_BAD_STATE"
            elif msg_epoch != self.epoch or self.accepted_epoch[rank] != self.epoch:
                result = "REJECT_STALE_EPOCH"
            elif rank not in self.durable or not self.topology_ok[rank] or self.corrupt[rank]:
                result = "REJECT_PRECONDITION"
            else:
                self.prepared.add(rank)
                result = "OK"
        elif kind == "cancel":
            if self.phase == "COMMITTED":
                result = "ALREADY_COMMITTED"
            elif self.phase == "ABORTED":
                result = "ALREADY_ABORTED"
            elif self.phase in {"IDLE", "OPEN"}:
                self.phase = "ABORTED"
                self.cancelled = True
                result = "ABORTED"
            else:
                raise AssertionError(f"unknown phase {self.phase}")
        elif kind == "commit":
            if self.phase == "COMMITTED":
                result = "ALREADY_COMMITTED"
            elif self.phase != "OPEN" or self.cancelled:
                result = "REJECT_TERMINAL_OR_BAD_STATE"
            elif self.durable != RANKS or self.prepared != RANKS:
                result = "REJECT_PARTIAL"
            elif any(not self.topology_ok[r] or self.corrupt[r] for r in RANKS):
                result = "REJECT_INVALID_STATE"
            elif any(self.accepted_epoch[r] != self.epoch for r in RANKS):
                result = "REJECT_STALE_RANK"
            else:
                self.phase = "COMMITTED"
                result = "COMMITTED"
        elif kind == "break_topology":
            self.topology_ok[self._rank(event)] = False
            self.last_read = "NONE"
            result = "OK"
        elif kind == "repair_topology":
            self.topology_ok[self._rank(event)] = True
            self.last_read = "NONE"
            result = "OK"
        elif kind == "corrupt":
            self.corrupt[self._rank(event)] = True
            self.last_read = "NONE"
            result = "OK"
        elif kind == "repair":
            self.corrupt[self._rank(event)] = False
            self.last_read = "NONE"
            result = "OK"
        elif kind == "read":
            valid = (
                self.phase == "COMMITTED"
                and self.durable == RANKS
                and self.prepared == RANKS
                and all(self.topology_ok[r] and not self.corrupt[r] for r in RANKS)
            )
            self.last_read = "ACCEPTED" if valid else "REJECTED"
            result = self.last_read
        elif kind == "single_node_decision":
            required = ["full_weights", "model_fits", "supported_topology", "complete_state", "fenced_old_generation"]
            permitted = all(bool(event.get(k)) for k in required)
            result = "PERMIT" if permitted else "REJECT"
        else:
            raise ValueError(f"unknown event kind: {kind!r}")

        self.check_invariants()
        return result

    def _rank(self, event: dict[str, Any]) -> int:
        rank = int(event["rank"])
        if rank not in RANKS:
            raise ValueError(f"invalid rank: {rank}")
        return rank

    def _current_rank_can_mutate(self, rank: int) -> bool:
        return (
            self.accepted_epoch[rank] == self.epoch
            and self.topology_ok[rank]
            and not self.corrupt[rank]
        )

    def check_invariants(self) -> None:
        assert self.durable <= RANKS
        assert self.prepared <= RANKS
        assert all(v <= self.epoch for v in self.accepted_epoch.values())
        if self.phase == "COMMITTED":
            assert not self.cancelled
            assert self.durable == RANKS
            assert self.prepared == RANKS
        if self.last_read == "ACCEPTED":
            assert self.phase == "COMMITTED"
            assert self.durable == RANKS and self.prepared == RANKS
            assert all(self.topology_ok[r] and not self.corrupt[r] for r in RANKS)


def run_trace(path: Path) -> list[str]:
    events = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(events, list):
        raise ValueError("trace must be a JSON array")
    model = Model()
    results = [model.apply(event) for event in events]
    return results


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {argv[0]} TRACE.json", file=sys.stderr)
        return 2
    path = Path(argv[1])
    results = run_trace(path)
    for i, result in enumerate(results):
        print(f"{i:03d} {result}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
