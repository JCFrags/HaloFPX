------------------------------ MODULE HaloKV ------------------------------
EXTENDS Naturals, FiniteSets, TLC

CONSTANTS Ranks, MaxEpoch

ASSUME /\ Ranks # {}
       /\ Cardinality(Ranks) = 2
       /\ MaxEpoch \in Nat

Phases == {"Idle", "Open", "Committed", "Aborted"}
ReadResults == {"None", "Accepted", "Rejected"}
MsgKinds == {"Prepared"}
NullCert == [valid |-> FALSE, certEpoch |-> 0, rankSet |-> {}]

VARIABLES epoch,
          phase,
          durable,
          prepared,
          cancelled,
          online,
          acceptedEpoch,
          topologyOK,
          corrupt,
          messages,
          commitCert,
          readResult

vars == <<epoch, phase, durable, prepared, cancelled, online,
          acceptedEpoch, topologyOK, corrupt, messages, commitCert, readResult>>

MessageSet == [kind : MsgKinds, rank : Ranks, msgEpoch : 0..MaxEpoch]
CertificateSet == [valid : {TRUE}, certEpoch : 0..MaxEpoch, rankSet : SUBSET Ranks]

TypeOK ==
  /\ epoch \in 0..MaxEpoch
  /\ phase \in Phases
  /\ durable \subseteq Ranks
  /\ prepared \subseteq Ranks
  /\ cancelled \in BOOLEAN
  /\ online \in [Ranks -> BOOLEAN]
  /\ acceptedEpoch \in [Ranks -> 0..MaxEpoch]
  /\ topologyOK \in [Ranks -> BOOLEAN]
  /\ corrupt \in [Ranks -> BOOLEAN]
  /\ messages \subseteq MessageSet
  /\ commitCert \in {NullCert} \cup CertificateSet
  /\ readResult \in ReadResults

Init ==
  /\ epoch = 0
  /\ phase = "Idle"
  /\ durable = {}
  /\ prepared = {}
  /\ cancelled = FALSE
  /\ online = [r \in Ranks |-> TRUE]
  /\ acceptedEpoch = [r \in Ranks |-> 0]
  /\ topologyOK = [r \in Ranks |-> TRUE]
  /\ corrupt = [r \in Ranks |-> FALSE]
  /\ messages = {}
  /\ commitCert = NullCert
  /\ readResult = "None"

BumpEpoch ==
  /\ epoch < MaxEpoch
  /\ epoch' = epoch + 1
  /\ phase' = "Idle"
  /\ durable' = {}
  /\ prepared' = {}
  /\ cancelled' = FALSE
  /\ commitCert' = NullCert
  /\ readResult' = "None"
  /\ UNCHANGED <<online, acceptedEpoch, topologyOK, corrupt, messages>>

AcceptEpoch(r) ==
  /\ r \in Ranks
  /\ online[r]
  /\ acceptedEpoch[r] < epoch
  /\ acceptedEpoch' = [acceptedEpoch EXCEPT ![r] = epoch]
  /\ UNCHANGED <<epoch, phase, durable, prepared, cancelled, online,
                  topologyOK, corrupt, messages, commitCert, readResult>>

Begin ==
  /\ phase = "Idle"
  /\ phase' = "Open"
  /\ UNCHANGED <<epoch, durable, prepared, cancelled, online, acceptedEpoch,
                  topologyOK, corrupt, messages, commitCert, readResult>>

WriteRank(r) ==
  /\ r \in Ranks
  /\ phase = "Open"
  /\ online[r]
  /\ acceptedEpoch[r] = epoch
  /\ topologyOK[r]
  /\ ~corrupt[r]
  /\ durable' = durable \cup {r}
  /\ UNCHANGED <<epoch, phase, prepared, cancelled, online, acceptedEpoch,
                  topologyOK, corrupt, messages, commitCert, readResult>>

SendPrepared(r) ==
  /\ r \in durable
  /\ messages' = messages \cup {[kind |-> "Prepared", rank |-> r, msgEpoch |-> epoch]}
  /\ UNCHANGED <<epoch, phase, durable, prepared, cancelled, online, acceptedEpoch,
                  topologyOK, corrupt, commitCert, readResult>>

DeliverPrepared(m) ==
  /\ m \in messages
  /\ messages' = messages \ {m}
  /\ prepared' =
       IF /\ phase = "Open"
          /\ ~cancelled
          /\ m.msgEpoch = epoch
          /\ acceptedEpoch[m.rank] = epoch
          /\ m.rank \in durable
          /\ topologyOK[m.rank]
          /\ ~corrupt[m.rank]
       THEN prepared \cup {m.rank}
       ELSE prepared
  /\ UNCHANGED <<epoch, phase, durable, cancelled, online, acceptedEpoch,
                  topologyOK, corrupt, commitCert, readResult>>

Commit ==
  /\ phase = "Open"
  /\ ~cancelled
  /\ durable = Ranks
  /\ prepared = Ranks
  /\ \A r \in Ranks : acceptedEpoch[r] = epoch
  /\ \A r \in Ranks : topologyOK[r]
  /\ \A r \in Ranks : ~corrupt[r]
  /\ phase' = "Committed"
  /\ commitCert' = [valid |-> TRUE, certEpoch |-> epoch, rankSet |-> Ranks]
  /\ UNCHANGED <<epoch, durable, prepared, cancelled, online, acceptedEpoch,
                  topologyOK, corrupt, messages, readResult>>

Cancel ==
  /\ phase = "Open"
  /\ cancelled' = TRUE
  /\ phase' = "Aborted"
  /\ UNCHANGED <<epoch, durable, prepared, online, acceptedEpoch, topologyOK,
                  corrupt, messages, commitCert, readResult>>

Crash(r) ==
  /\ r \in Ranks
  /\ online[r]
  /\ online' = [online EXCEPT ![r] = FALSE]
  /\ UNCHANGED <<epoch, phase, durable, prepared, cancelled, acceptedEpoch,
                  topologyOK, corrupt, messages, commitCert, readResult>>

Recover(r) ==
  /\ r \in Ranks
  /\ ~online[r]
  /\ online' = [online EXCEPT ![r] = TRUE]
  /\ UNCHANGED <<epoch, phase, durable, prepared, cancelled, acceptedEpoch,
                  topologyOK, corrupt, messages, commitCert, readResult>>

BreakTopology(r) ==
  /\ r \in Ranks
  /\ topologyOK[r]
  /\ topologyOK' = [topologyOK EXCEPT ![r] = FALSE]
  /\ readResult' = "None"
  /\ UNCHANGED <<epoch, phase, durable, prepared, cancelled, online, acceptedEpoch,
                  corrupt, messages, commitCert>>

RepairTopology(r) ==
  /\ r \in Ranks
  /\ ~topologyOK[r]
  /\ topologyOK' = [topologyOK EXCEPT ![r] = TRUE]
  /\ readResult' = "None"
  /\ UNCHANGED <<epoch, phase, durable, prepared, cancelled, online, acceptedEpoch,
                  corrupt, messages, commitCert>>

CorruptRank(r) ==
  /\ r \in Ranks
  /\ ~corrupt[r]
  /\ corrupt' = [corrupt EXCEPT ![r] = TRUE]
  /\ readResult' = "None"
  /\ UNCHANGED <<epoch, phase, durable, prepared, cancelled, online, acceptedEpoch,
                  topologyOK, messages, commitCert>>

RepairRank(r) ==
  /\ r \in Ranks
  /\ corrupt[r]
  /\ corrupt' = [corrupt EXCEPT ![r] = FALSE]
  /\ readResult' = "None"
  /\ UNCHANGED <<epoch, phase, durable, prepared, cancelled, online, acceptedEpoch,
                  topologyOK, messages, commitCert>>

TryRead ==
  /\ readResult' =
       IF /\ phase = "Committed"
          /\ commitCert.valid
          /\ commitCert.certEpoch = epoch
          /\ commitCert.rankSet = Ranks
          /\ durable = Ranks
          /\ prepared = Ranks
          /\ \A r \in Ranks : topologyOK[r]
          /\ \A r \in Ranks : ~corrupt[r]
       THEN "Accepted"
       ELSE "Rejected"
  /\ UNCHANGED <<epoch, phase, durable, prepared, cancelled, online, acceptedEpoch,
                  topologyOK, corrupt, messages, commitCert>>

Next ==
  \/ BumpEpoch
  \/ Begin
  \/ Commit
  \/ Cancel
  \/ TryRead
  \/ \E r \in Ranks : AcceptEpoch(r)
  \/ \E r \in Ranks : WriteRank(r)
  \/ \E r \in Ranks : SendPrepared(r)
  \/ \E m \in messages : DeliverPrepared(m)
  \/ \E r \in Ranks : Crash(r)
  \/ \E r \in Ranks : Recover(r)
  \/ \E r \in Ranks : BreakTopology(r)
  \/ \E r \in Ranks : RepairTopology(r)
  \/ \E r \in Ranks : CorruptRank(r)
  \/ \E r \in Ranks : RepairRank(r)

Spec == Init /\ [][Next]_vars

CommitHasAllRanks ==
  phase = "Committed" => /\ durable = Ranks
                           /\ prepared = Ranks
                           /\ commitCert.valid
                           /\ commitCert.rankSet = Ranks

CommitNotCancelled == phase = "Committed" => ~cancelled

PreparedRanksAreCurrent ==
  \A r \in prepared : acceptedEpoch[r] = epoch

EpochFence ==
  \A r \in Ranks : acceptedEpoch[r] <= epoch

AcceptedReadIsValid ==
  readResult = "Accepted" =>
    /\ phase = "Committed"
    /\ commitCert.valid
    /\ commitCert.certEpoch = epoch
    /\ commitCert.rankSet = Ranks
    /\ durable = Ranks
    /\ prepared = Ranks
    /\ \A r \in Ranks : topologyOK[r]
    /\ \A r \in Ranks : ~corrupt[r]

=============================================================================
