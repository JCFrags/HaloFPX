// Research starter sketch; adapt to the installed P release before compiling.

type tRank = int;
type tCtx = (epoch: int, op: int);
type tPrepared = (ctx: tCtx, rank: tRank);

event eBegin: tCtx;
event eDurable;
event ePrepared: tPrepared;
event eCommit: tCtx;
event eCancel: tCtx;
event eFence: int;
event eCommittedObserved: (ctx: tCtx, ranks: set[tRank]);

machine Rank {
  var id: tRank;
  var currentEpoch: int;
  var currentOp: int;

  start state Init {
    entry (payload: (rank: tRank, epoch: int)) {
      id = payload.rank;
      currentEpoch = payload.epoch;
      goto Ready;
    }
  }

  state Ready {
    on eBegin do (ctx: tCtx) {
      if (ctx.epoch == currentEpoch) {
        currentOp = ctx.op;
        goto Snapshotting;
      }
    }
    on eFence do (epoch: int) {
      if (epoch > currentEpoch) {
        currentEpoch = epoch;
        goto Fenced;
      }
    }
  }

  state Snapshotting {
    on eDurable do {
      send parent, ePrepared, (ctx = (epoch = currentEpoch, op = currentOp), rank = id);
      goto Prepared;
    }
    on eCancel goto Ready;
    on eFence goto Fenced;
  }

  state Prepared {
    on eCommit goto Ready;
    on eCancel goto Ready;
    on eFence goto Fenced;
  }

  state Fenced { }
}

machine Coordinator {
  var current: tCtx;
  var prepared: set[tRank];
  var ranks: set[tRank];

  start state Init {
    entry {
      ranks = default(set[tRank]);
      ranks += (0);
      ranks += (1);
      goto Idle;
    }
  }

  state Idle {
    on eBegin do (ctx: tCtx) {
      current = ctx;
      prepared = default(set[tRank]);
      goto Open;
    }
  }

  state Open {
    on ePrepared do (p: tPrepared) {
      if (p.ctx == current && p.rank in ranks) {
        prepared += (p.rank);
        if (prepared == ranks) {
          send this, eCommit, current;
        }
      }
    }
    on eCommit do (ctx: tCtx) {
      assert ctx == current;
      assert prepared == ranks;
      announce eCommittedObserved, (ctx = current, ranks = prepared);
      goto Committed;
    }
    on eCancel goto Aborted;
  }

  state Committed { }
  state Aborted { }
}

spec CommitMonitor observes eCommittedObserved {
  start state Watching {
    on eCommittedObserved do (p: (ctx: tCtx, ranks: set[tRank])) {
      assert sizeof(p.ranks) == 2;
      assert 0 in p.ranks;
      assert 1 in p.ranks;
    }
  }
}
