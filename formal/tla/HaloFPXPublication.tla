-------------------------- MODULE HaloFPXPublication --------------------------
EXTENDS Naturals, FiniteSets, TLC

(***************************************************************************
Target-owned bounded model for ADR-0004. It abstracts bytes, cryptography,
paths, real filesystems, and time. It models publication ordering, exact
protected-anchor identity, authority fencing, fail-closed recovery, and
lineage-local state. A model-checking pass is not implementation proof.
***************************************************************************)

CONSTANTS
    \* @type: Set(Str);
    Ranks,
    \* @type: Set(Str);
    Lineages,
    \* @type: Set(Str);
    Writers,
    \* @type: Int;
    MaxGeneration,
    \* @type: Int;
    MaxAuthorityEpoch,
    \* @type: Bool;
    EnableFaults,
    \* @type: Str;
    BrokenMode,
    \* @type: Str;
    NoWriter,
    \* @type: Int;
    NoGeneration,
    \* @type: Int;
    Miss,
    \* @type: Int;
    OtherDigest

Generations == 0..MaxGeneration
PositiveGenerations == 1..MaxGeneration
ObjectPhases == {"Absent", "Created", "Written", "Verified", "FileDurable",
                  "Named", "DirDurable", "Corrupt", "Missing"}
ManifestPhases == {"Absent", "Written", "Verified", "FileDurable", "Named",
                    "DirDurable", "Corrupt", "Missing"}
Contents == {"NoContent", "Expected", "Other"}

Identity(lineage, g, predDigest, epoch) ==
    [lineage |-> lineage,
     generation |-> g,
     manifestDigest |-> g,
     predecessorDigest |-> predDigest,
     policyEpoch |-> 0,
     keyGeneration |-> 0,
     authorityEpoch |-> epoch]

NullIdentity(lineage) == Identity(lineage, NoGeneration, NoGeneration, 0)

VARIABLES
    \* @type: Str;
    authority,
    \* @type: Int;
    authorityEpoch,
    \* @type: Str -> Bool;
    writerAlive,
    \* @type: Str -> Str;
    attemptOwner,
    \* @type: Str -> Int;
    attemptGeneration,
    \* @type: Str -> Int;
    attemptPredecessor,
    \* @type: Str -> (Int -> (Str -> Str));
    objectPhase,
    \* @type: Str -> (Int -> (Str -> Str));
    objectContent,
    \* @type: Str -> (Int -> Str);
    manifestPhase,
    \* @type: Str -> (Int -> Str);
    manifestContent,
    \* @type: Str -> (Int -> Int);
    manifestPredecessor,
    \* @type: Str -> (Int -> { lineage: Str, generation: Int, manifestDigest: Int, predecessorDigest: Int, policyEpoch: Int, keyGeneration: Int, authorityEpoch: Int });
    manifestIdentity,
    \* @type: Str -> (Int -> Bool);
    fingerprintOK,
    \* @type: Str -> Int;
    liveAnchor,
    \* @type: Str -> { lineage: Str, generation: Int, manifestDigest: Int, predecessorDigest: Int, policyEpoch: Int, keyGeneration: Int, authorityEpoch: Int };
    liveAnchorIdentity,
    \* @type: Str -> Int;
    durableAnchor,
    \* @type: Str -> { lineage: Str, generation: Int, manifestDigest: Int, predecessorDigest: Int, policyEpoch: Int, keyGeneration: Int, authorityEpoch: Int };
    durableAnchorIdentity,
    \* @type: Str -> Bool;
    anchorSyncPending,
    \* @type: Str -> Set(Int);
    acknowledgements,
    \* @type: Str -> Set(Int);
    abandoned,
    \* @type: Str -> Bool;
    fatal,
    \* @type: Str -> Int;
    recovered,
    \* @type: Str -> (Str -> Int);
    recoveryObjectGeneration,
    \* @type: Str -> Bool;
    lineageEnabled,
    \* @type: Str -> Int;
    highWater

vars == << authority, authorityEpoch, writerAlive,
           attemptOwner, attemptGeneration, attemptPredecessor,
           objectPhase, objectContent, manifestPhase, manifestContent,
           manifestPredecessor, manifestIdentity, fingerprintOK,
           liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
           anchorSyncPending, acknowledgements, abandoned, fatal,
           recovered, recoveryObjectGeneration, lineageEnabled, highWater >>

ASSUME /\ Ranks # {} /\ Lineages # {} /\ Writers # {}
       /\ MaxGeneration >= 1 /\ MaxAuthorityEpoch >= 0
       /\ EnableFaults \in BOOLEAN
       /\ BrokenMode \in {"None", "AckEarly", "RecoverNewest", "MixedRecovery", "ReplayAnchor", "CrossLineage"}
       /\ NoWriter \notin Writers
       /\ NoGeneration > MaxGeneration /\ Miss > MaxGeneration /\ OtherDigest > MaxGeneration
       /\ NoGeneration # Miss /\ NoGeneration # OtherDigest /\ Miss # OtherDigest

BasePhase(g) == IF g = 0 THEN "DirDurable" ELSE "Absent"
BaseContent(g) == IF g = 0 THEN "Expected" ELSE "NoContent"
BaseIdentity(l, g) == IF g = 0 THEN Identity(l, 0, NoGeneration, 0) ELSE NullIdentity(l)

Init ==
    /\ authority \in Writers
    /\ authorityEpoch = 0
    /\ writerAlive = [w \in Writers |-> TRUE]
    /\ attemptOwner = [l \in Lineages |-> NoWriter]
    /\ attemptGeneration = [l \in Lineages |-> 0]
    /\ attemptPredecessor = [l \in Lineages |-> NoGeneration]
    /\ objectPhase = [l \in Lineages |-> [g \in Generations |-> [r \in Ranks |-> BasePhase(g)]]]
    /\ objectContent = [l \in Lineages |-> [g \in Generations |-> [r \in Ranks |-> BaseContent(g)]]]
    /\ manifestPhase = [l \in Lineages |-> [g \in Generations |-> BasePhase(g)]]
    /\ manifestContent = [l \in Lineages |-> [g \in Generations |-> BaseContent(g)]]
    /\ manifestPredecessor = [l \in Lineages |-> [g \in Generations |-> IF g = 0 THEN NoGeneration ELSE g - 1]]
    /\ manifestIdentity = [l \in Lineages |-> [g \in Generations |-> BaseIdentity(l, g)]]
    /\ fingerprintOK = [l \in Lineages |-> [g \in Generations |-> TRUE]]
    /\ liveAnchor = [l \in Lineages |-> 0]
    /\ liveAnchorIdentity = [l \in Lineages |-> BaseIdentity(l, 0)]
    /\ durableAnchor = [l \in Lineages |-> 0]
    /\ durableAnchorIdentity = [l \in Lineages |-> BaseIdentity(l, 0)]
    /\ anchorSyncPending = [l \in Lineages |-> FALSE]
    /\ acknowledgements = [l \in Lineages |-> {}]
    /\ abandoned = [l \in Lineages |-> {}]
    /\ fatal = [l \in Lineages |-> FALSE]
    /\ recovered = [l \in Lineages |-> NoGeneration]
    /\ recoveryObjectGeneration = [l \in Lineages |-> [r \in Ranks |-> NoGeneration]]
    /\ lineageEnabled = [l \in Lineages |-> TRUE]
    /\ highWater = [l \in Lineages |-> 0]

Active(l, g) == attemptOwner[l] # NoWriter /\ attemptGeneration[l] = g
WriterCanAdvance(l, g) ==
    /\ Active(l, g)
    /\ attemptOwner[l] = authority
    /\ writerAlive[authority]
AllObjectsAt(l, g, phase) == \A r \in Ranks : objectPhase[l][g][r] = phase
AllObjectsDurable(l, g) == AllObjectsAt(l, g, "DirDurable")
ManifestDurable(l, g) == manifestPhase[l][g] = "DirDurable"

EntryValid(l, g) ==
    /\ g \in Generations
    /\ ManifestDurable(l, g)
    /\ manifestContent[l][g] = "Expected"
    /\ fingerprintOK[l][g]
    /\ AllObjectsDurable(l, g)
    /\ \A r \in Ranks : objectContent[l][g][r] = "Expected"
    /\ manifestIdentity[l][g].generation = g
    /\ manifestIdentity[l][g].lineage = l
    /\ manifestIdentity[l][g].manifestDigest = g
    /\ manifestIdentity[l][g].policyEpoch = 0
    /\ manifestIdentity[l][g].keyGeneration = 0
    /\ manifestIdentity[l][g].authorityEpoch \in 0..MaxAuthorityEpoch

ChainValid(l, g) ==
    /\ \A p \in Generations : p <= g => EntryValid(l, p)
    /\ manifestPredecessor[l][0] = NoGeneration
    /\ manifestIdentity[l][0].predecessorDigest = NoGeneration
    /\ \A p \in PositiveGenerations : p <= g =>
          /\ manifestPredecessor[l][p] = p - 1
          /\ manifestIdentity[l][p].predecessorDigest = manifestIdentity[l][p - 1].manifestDigest

SelectedValid(l, g) == g \in Generations /\ EntryValid(l, g) /\ ChainValid(l, g)

AnchorIdentityExact(l, g, anchorIdentity) ==
    /\ anchorIdentity = manifestIdentity[l][g]
    /\ anchorIdentity.lineage = l
    /\ anchorIdentity.generation = g
    /\ anchorIdentity.manifestDigest = g

DurableAnchorValid(l) ==
    /\ SelectedValid(l, durableAnchor[l])
    /\ AnchorIdentityExact(l, durableAnchor[l], durableAnchorIdentity[l])

AnchorPrerequisites(l, g) ==
    /\ SelectedValid(l, g)
    /\ WriterCanAdvance(l, g)
    /\ attemptPredecessor[l] = durableAnchor[l]
    /\ manifestIdentity[l][g].predecessorDigest = durableAnchorIdentity[l].manifestDigest
    /\ g = durableAnchor[l] + 1

UnchangedStorage ==
    UNCHANGED << objectPhase, objectContent, manifestPhase, manifestContent,
                 manifestPredecessor, manifestIdentity, fingerprintOK >>
UnchangedAnchors ==
    UNCHANGED << liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                 anchorSyncPending, highWater >>
UnchangedRecovery ==
    UNCHANGED << recovered, recoveryObjectGeneration, lineageEnabled >>
UnchangedControl ==
    UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner,
                 attemptGeneration, attemptPredecessor, acknowledgements,
                 abandoned, fatal >>

BeginAttempt(w, l, g) ==
    /\ w = authority /\ writerAlive[w] /\ attemptOwner[l] = NoWriter /\ ~fatal[l]
    /\ g \in PositiveGenerations /\ g = durableAnchor[l] + 1 /\ DurableAnchorValid(l)
    /\ attemptOwner' = [attemptOwner EXCEPT ![l] = w]
    /\ attemptGeneration' = [attemptGeneration EXCEPT ![l] = g]
    /\ attemptPredecessor' = [attemptPredecessor EXCEPT ![l] = durableAnchor[l]]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, objectPhase, objectContent,
                     manifestPhase, manifestContent, manifestPredecessor, manifestIdentity,
                     fingerprintOK, liveAnchor, liveAnchorIdentity, durableAnchor,
                     durableAnchorIdentity, anchorSyncPending, acknowledgements, abandoned,
                     fatal, recovered, recoveryObjectGeneration, lineageEnabled, highWater >>

CreateObject(l, g, r) ==
    /\ WriterCanAdvance(l, g) /\ objectPhase[l][g][r] = "Absent"
    /\ objectContent[l][g][r] = "NoContent"
    /\ objectPhase' = [objectPhase EXCEPT ![l][g][r] = "Created"]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectContent, manifestPhase, manifestContent,
                     manifestPredecessor, manifestIdentity, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, abandoned, fatal, recovered, recoveryObjectGeneration,
                     lineageEnabled, highWater >>

AdvanceObject(l, g, r, fromPhase, toPhase) ==
    /\ WriterCanAdvance(l, g) /\ objectPhase[l][g][r] = fromPhase
    /\ objectPhase' = [objectPhase EXCEPT ![l][g][r] = toPhase]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectContent, manifestPhase, manifestContent,
                     manifestPredecessor, manifestIdentity, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, abandoned, fatal, recovered, recoveryObjectGeneration,
                     lineageEnabled, highWater >>

PublishObjectNoReplace(l, g, r) ==
    /\ WriterCanAdvance(l, g) /\ objectPhase[l][g][r] = "FileDurable"
    /\ objectContent[l][g][r] = "NoContent"
    /\ objectPhase' = [objectPhase EXCEPT ![l][g][r] = "Named"]
    /\ objectContent' = [objectContent EXCEPT ![l][g][r] = "Expected"]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, manifestPhase, manifestContent, manifestPredecessor,
                     manifestIdentity, fingerprintOK, liveAnchor, liveAnchorIdentity,
                     durableAnchor, durableAnchorIdentity, anchorSyncPending, acknowledgements,
                     abandoned, fatal, recovered, recoveryObjectGeneration, lineageEnabled, highWater >>

InjectExistingObject(l, g, r, content) ==
    /\ EnableFaults /\ WriterCanAdvance(l, g)
    /\ objectPhase[l][g][r] = "FileDurable" /\ objectContent[l][g][r] = "NoContent"
    /\ content \in {"Expected", "Other"}
    /\ objectContent' = [objectContent EXCEPT ![l][g][r] = content]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, manifestPhase, manifestContent,
                     manifestPredecessor, manifestIdentity, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, abandoned, fatal, recovered, recoveryObjectGeneration,
                     lineageEnabled, highWater >>

VerifyExistingObject(l, g, r) ==
    /\ WriterCanAdvance(l, g) /\ objectPhase[l][g][r] = "FileDurable"
    /\ objectContent[l][g][r] = "Expected"
    /\ objectPhase' = [objectPhase EXCEPT ![l][g][r] = "Named"]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectContent, manifestPhase, manifestContent,
                     manifestPredecessor, manifestIdentity, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, abandoned, fatal, recovered, recoveryObjectGeneration,
                     lineageEnabled, highWater >>

RejectObjectCollision(l, g, r) ==
    /\ WriterCanAdvance(l, g) /\ objectPhase[l][g][r] = "FileDurable"
    /\ objectContent[l][g][r] = "Other"
    /\ fatal' = [fatal EXCEPT ![l] = TRUE]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                     anchorSyncPending, acknowledgements, abandoned, recovered,
                     recoveryObjectGeneration, lineageEnabled, highWater >>

WriteManifest(l, g) ==
    /\ WriterCanAdvance(l, g) /\ AllObjectsDurable(l, g)
    /\ manifestPhase[l][g] = "Absent"
    /\ manifestPhase' = [manifestPhase EXCEPT ![l][g] = "Written"]
    /\ manifestPredecessor' = [manifestPredecessor EXCEPT ![l][g] = attemptPredecessor[l]]
    /\ manifestIdentity' = [manifestIdentity EXCEPT
          ![l][g] = Identity(l, g, durableAnchorIdentity[l].manifestDigest, authorityEpoch)]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestContent,
                     fingerprintOK, liveAnchor, liveAnchorIdentity, durableAnchor,
                     durableAnchorIdentity, anchorSyncPending, acknowledgements, abandoned,
                     fatal, recovered, recoveryObjectGeneration, lineageEnabled, highWater >>

AdvanceManifest(l, g, fromPhase, toPhase) ==
    /\ WriterCanAdvance(l, g) /\ manifestPhase[l][g] = fromPhase
    /\ manifestPhase' = [manifestPhase EXCEPT ![l][g] = toPhase]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestContent,
                     manifestPredecessor, manifestIdentity, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, abandoned, fatal, recovered, recoveryObjectGeneration,
                     lineageEnabled, highWater >>

PublishManifestNoReplace(l, g) ==
    /\ WriterCanAdvance(l, g) /\ manifestPhase[l][g] = "FileDurable"
    /\ manifestContent[l][g] = "NoContent"
    /\ manifestPhase' = [manifestPhase EXCEPT ![l][g] = "Named"]
    /\ manifestContent' = [manifestContent EXCEPT ![l][g] = "Expected"]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPredecessor,
                     manifestIdentity, fingerprintOK, liveAnchor, liveAnchorIdentity,
                     durableAnchor, durableAnchorIdentity, anchorSyncPending, acknowledgements,
                     abandoned, fatal, recovered, recoveryObjectGeneration, lineageEnabled, highWater >>

InjectExistingManifest(l, g, content) ==
    /\ EnableFaults /\ WriterCanAdvance(l, g)
    /\ manifestPhase[l][g] = "FileDurable" /\ manifestContent[l][g] = "NoContent"
    /\ content \in {"Expected", "Other"}
    /\ manifestContent' = [manifestContent EXCEPT ![l][g] = content]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestPredecessor, manifestIdentity, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, abandoned, fatal, recovered, recoveryObjectGeneration,
                     lineageEnabled, highWater >>

VerifyExistingManifest(l, g) ==
    /\ WriterCanAdvance(l, g) /\ manifestPhase[l][g] = "FileDurable"
    /\ manifestContent[l][g] = "Expected"
    /\ manifestPhase' = [manifestPhase EXCEPT ![l][g] = "Named"]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestContent,
                     manifestPredecessor, manifestIdentity, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, abandoned, fatal, recovered, recoveryObjectGeneration,
                     lineageEnabled, highWater >>

RejectManifestCollision(l, g) ==
    /\ WriterCanAdvance(l, g) /\ manifestPhase[l][g] = "FileDurable"
    /\ manifestContent[l][g] = "Other"
    /\ fatal' = [fatal EXCEPT ![l] = TRUE]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                     anchorSyncPending, acknowledgements, abandoned, recovered,
                     recoveryObjectGeneration, lineageEnabled, highWater >>

AtomicReplaceAnchor(l) ==
    LET g == attemptGeneration[l] IN
    /\ ~anchorSyncPending[l] /\ AnchorPrerequisites(l, g)
    /\ \A other \in Lineages \ {l} : ~anchorSyncPending[other]
    /\ liveAnchor' = [liveAnchor EXCEPT ![l] = g]
    /\ liveAnchorIdentity' = [liveAnchorIdentity EXCEPT ![l] = manifestIdentity[l][g]]
    /\ anchorSyncPending' = [anchorSyncPending EXCEPT ![l] = TRUE]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     durableAnchor, durableAnchorIdentity, acknowledgements, abandoned, fatal,
                     recovered, recoveryObjectGeneration, lineageEnabled, highWater >>

SyncAnchor(l) ==
    /\ anchorSyncPending[l] /\ SelectedValid(l, liveAnchor[l])
    /\ AnchorIdentityExact(l, liveAnchor[l], liveAnchorIdentity[l])
    /\ durableAnchor' = [durableAnchor EXCEPT ![l] = liveAnchor[l]]
    /\ durableAnchorIdentity' = [durableAnchorIdentity EXCEPT ![l] = liveAnchorIdentity[l]]
    /\ highWater' = [highWater EXCEPT ![l] = liveAnchor[l]]
    /\ anchorSyncPending' = [anchorSyncPending EXCEPT ![l] = FALSE]
    /\ attemptOwner' = [attemptOwner EXCEPT ![l] = NoWriter]
    /\ recovered' = [recovered EXCEPT ![l] = NoGeneration]
    /\ recoveryObjectGeneration' = [recoveryObjectGeneration EXCEPT ![l] = [r \in Ranks |-> NoGeneration]]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, acknowledgements, abandoned, fatal, lineageEnabled >>

AckDurability(l, g) ==
    /\ g \in PositiveGenerations
    /\ IF BrokenMode = "AckEarly" THEN liveAnchor[l] = g
          ELSE /\ ~anchorSyncPending[l] /\ durableAnchor[l] = g /\ DurableAnchorValid(l)
    /\ acknowledgements' = [acknowledgements EXCEPT ![l] = @ \cup {g}]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                     anchorSyncPending, abandoned, fatal, recovered, recoveryObjectGeneration,
                     lineageEnabled, highWater >>

AbandonGeneration(l) ==
    LET g == attemptGeneration[l] IN
    /\ attemptOwner[l] # NoWriter /\ ~anchorSyncPending[l]
    /\ abandoned' = [abandoned EXCEPT ![l] = @ \cup {g}]
    /\ attemptOwner' = [attemptOwner EXCEPT ![l] = NoWriter]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                     anchorSyncPending, acknowledgements, fatal, recovered,
                     recoveryObjectGeneration, lineageEnabled, highWater >>

CrashBeforeAnchorSyncOld(l) ==
    /\ EnableFaults /\ anchorSyncPending[l] /\ writerAlive[authority]
    /\ \A other \in Lineages \ {l} : ~anchorSyncPending[other]
    /\ writerAlive' = [writerAlive EXCEPT ![authority] = FALSE]
    /\ liveAnchor' = [liveAnchor EXCEPT ![l] = durableAnchor[l]]
    /\ liveAnchorIdentity' = [liveAnchorIdentity EXCEPT ![l] = durableAnchorIdentity[l]]
    /\ anchorSyncPending' = [anchorSyncPending EXCEPT ![l] = FALSE]
    /\ abandoned' = [other \in Lineages |->
          IF attemptOwner[other] = authority THEN abandoned[other] \cup {attemptGeneration[other]}
          ELSE abandoned[other]]
    /\ attemptOwner' = [other \in Lineages |->
          IF attemptOwner[other] = authority THEN NoWriter ELSE attemptOwner[other]]
    /\ UNCHANGED << authority, authorityEpoch, attemptGeneration, attemptPredecessor,
                     objectPhase, objectContent, manifestPhase, manifestContent,
                     manifestPredecessor, manifestIdentity, fingerprintOK, durableAnchor,
                     durableAnchorIdentity, acknowledgements, fatal, recovered,
                     recoveryObjectGeneration, lineageEnabled, highWater >>

CrashBeforeAnchorSyncNew(l) ==
    /\ EnableFaults /\ anchorSyncPending[l] /\ writerAlive[authority]
    /\ \A other \in Lineages \ {l} : ~anchorSyncPending[other]
    /\ SelectedValid(l, liveAnchor[l])
    /\ AnchorIdentityExact(l, liveAnchor[l], liveAnchorIdentity[l])
    /\ writerAlive' = [writerAlive EXCEPT ![authority] = FALSE]
    /\ durableAnchor' = [durableAnchor EXCEPT ![l] = liveAnchor[l]]
    /\ durableAnchorIdentity' = [durableAnchorIdentity EXCEPT ![l] = liveAnchorIdentity[l]]
    /\ highWater' = [highWater EXCEPT ![l] = liveAnchor[l]]
    /\ anchorSyncPending' = [anchorSyncPending EXCEPT ![l] = FALSE]
    /\ abandoned' = [other \in Lineages |->
          IF attemptOwner[other] = authority THEN abandoned[other] \cup {attemptGeneration[other]}
          ELSE abandoned[other]]
    /\ attemptOwner' = [other \in Lineages |->
          IF attemptOwner[other] = authority THEN NoWriter ELSE attemptOwner[other]]
    /\ recovered' = [recovered EXCEPT ![l] = NoGeneration]
    /\ recoveryObjectGeneration' = [recoveryObjectGeneration EXCEPT ![l] = [r \in Ranks |-> NoGeneration]]
    /\ UNCHANGED << authority, authorityEpoch, attemptGeneration, attemptPredecessor,
                     objectPhase, objectContent, manifestPhase, manifestContent,
                     manifestPredecessor, manifestIdentity, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, acknowledgements, fatal, lineageEnabled >>

CrashAuthority ==
    /\ EnableFaults /\ writerAlive[authority]
    /\ \A l \in Lineages : ~anchorSyncPending[l]
    /\ writerAlive' = [writerAlive EXCEPT ![authority] = FALSE]
    /\ abandoned' = [l \in Lineages |->
          IF attemptOwner[l] = authority THEN abandoned[l] \cup {attemptGeneration[l]} ELSE abandoned[l]]
    /\ attemptOwner' = [l \in Lineages |-> IF attemptOwner[l] = authority THEN NoWriter ELSE attemptOwner[l]]
    /\ UNCHANGED << authority, authorityEpoch, attemptGeneration, attemptPredecessor,
                     objectPhase, objectContent, manifestPhase, manifestContent,
                     manifestPredecessor, manifestIdentity, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, fatal, recovered, recoveryObjectGeneration,
                     lineageEnabled, highWater >>

RestartWriter(w) ==
    /\ EnableFaults /\ w = authority /\ ~writerAlive[w]
    /\ writerAlive' = [writerAlive EXCEPT ![w] = TRUE]
    /\ UNCHANGED << authority, authorityEpoch, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                     anchorSyncPending, acknowledgements, abandoned, fatal, recovered,
                     recoveryObjectGeneration, lineageEnabled, highWater >>

TransferAuthority(w) ==
    /\ EnableFaults /\ w \in Writers /\ w # authority /\ ~writerAlive[authority]
    /\ authorityEpoch < MaxAuthorityEpoch
    /\ authority' = w /\ authorityEpoch' = authorityEpoch + 1
    /\ writerAlive' = [writerAlive EXCEPT ![w] = TRUE]
    /\ UNCHANGED << attemptOwner, attemptGeneration, attemptPredecessor, objectPhase,
                     objectContent, manifestPhase, manifestContent, manifestPredecessor,
                     manifestIdentity, fingerprintOK, liveAnchor, liveAnchorIdentity,
                     durableAnchor, durableAnchorIdentity, anchorSyncPending, acknowledgements,
                     abandoned, fatal, recovered, recoveryObjectGeneration,
                     lineageEnabled, highWater >>

ClearRecovery(l) ==
    /\ recovered' = [recovered EXCEPT ![l] = NoGeneration]
    /\ recoveryObjectGeneration' = [recoveryObjectGeneration EXCEPT ![l] = [r \in Ranks |-> NoGeneration]]
    /\ lineageEnabled' = [lineageEnabled EXCEPT ![l] = FALSE]

CorruptObject(l, g, r, phase) ==
    /\ EnableFaults /\ phase \in {"Corrupt", "Missing"}
    /\ objectPhase[l][g][r] = "DirDurable"
    /\ objectPhase' = [objectPhase EXCEPT ![l][g][r] = phase]
    /\ ClearRecovery(l)
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectContent, manifestPhase, manifestContent,
                     manifestPredecessor, manifestIdentity, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, abandoned, fatal, highWater >>

CorruptManifest(l, g, phase) ==
    /\ EnableFaults /\ phase \in {"Corrupt", "Missing"}
    /\ manifestPhase[l][g] = "DirDurable"
    /\ manifestPhase' = [manifestPhase EXCEPT ![l][g] = phase]
    /\ ClearRecovery(l)
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestContent,
                     manifestPredecessor, manifestIdentity, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, abandoned, fatal, highWater >>

StaleFingerprint(l, g) ==
    /\ EnableFaults /\ g \in PositiveGenerations /\ fingerprintOK[l][g]
    /\ fingerprintOK' = [fingerprintOK EXCEPT ![l][g] = FALSE]
    /\ ClearRecovery(l)
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, abandoned, fatal, highWater >>

MutateManifestIdentity(l, g, field) ==
    /\ EnableFaults /\ g \in PositiveGenerations /\ field \in {"Digest", "Predecessor"}
    /\ manifestPhase[l][g] = "DirDurable"
    /\ manifestIdentity' = [manifestIdentity EXCEPT
          ![l][g] = IF field = "Digest"
                     THEN [@ EXCEPT !.manifestDigest = OtherDigest]
                     ELSE [@ EXCEPT !.predecessorDigest = OtherDigest]]
    /\ ClearRecovery(l)
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, fingerprintOK, liveAnchor,
                     liveAnchorIdentity, durableAnchor, durableAnchorIdentity, anchorSyncPending,
                     acknowledgements, abandoned, fatal, highWater >>

ReplayCrossLineageAnchor(target, source) ==
    /\ EnableFaults /\ target # source
    /\ target \in Lineages /\ source \in Lineages
    /\ durableAnchor[target] = durableAnchor[source]
    /\ durableAnchorIdentity' = [durableAnchorIdentity EXCEPT
          ![target] = manifestIdentity[source][durableAnchor[source]]]
    /\ ClearRecovery(target)
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, anchorSyncPending,
                     acknowledgements, abandoned, fatal, highWater >>

RecoverSelected(l) ==
    /\ DurableAnchorValid(l)
    /\ recovered' = [recovered EXCEPT ![l] = durableAnchor[l]]
    /\ recoveryObjectGeneration' = [recoveryObjectGeneration EXCEPT ![l] = [r \in Ranks |-> durableAnchor[l]]]
    /\ lineageEnabled' = [lineageEnabled EXCEPT ![l] = TRUE]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                     anchorSyncPending, acknowledgements, abandoned, fatal, highWater >>

RejectSelected(l) ==
    /\ ~DurableAnchorValid(l)
    /\ recovered' = [recovered EXCEPT ![l] = Miss]
    /\ recoveryObjectGeneration' = [recoveryObjectGeneration EXCEPT ![l] = [r \in Ranks |-> NoGeneration]]
    /\ lineageEnabled' = [lineageEnabled EXCEPT ![l] = FALSE]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                     anchorSyncPending, acknowledgements, abandoned, fatal, highWater >>

RecomputeAfterMiss(l) ==
    /\ recovered[l] = Miss /\ ~lineageEnabled[l]
    /\ recovered' = [recovered EXCEPT ![l] = NoGeneration]
    /\ lineageEnabled' = [lineageEnabled EXCEPT ![l] = TRUE]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                     anchorSyncPending, acknowledgements, abandoned, fatal,
                     recoveryObjectGeneration, highWater >>

BrokenRecoverNewest(l, g) ==
    /\ BrokenMode = "RecoverNewest" /\ g \in PositiveGenerations /\ g # durableAnchor[l]
    /\ SelectedValid(l, g)
    /\ recovered' = [recovered EXCEPT ![l] = g]
    /\ recoveryObjectGeneration' = [recoveryObjectGeneration EXCEPT ![l] = [r \in Ranks |-> g]]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                     anchorSyncPending, acknowledgements, abandoned, fatal, lineageEnabled, highWater >>

BrokenMixedRecovery(l, r1, r2, g1, g2) ==
    /\ BrokenMode = "MixedRecovery" /\ r1 # r2 /\ g1 # g2
    /\ r1 \in Ranks /\ r2 \in Ranks /\ g1 \in Generations /\ g2 \in Generations
    /\ objectPhase[l][g1][r1] = "DirDurable" /\ objectPhase[l][g2][r2] = "DirDurable"
    /\ recovered' = [recovered EXCEPT ![l] = durableAnchor[l]]
    /\ recoveryObjectGeneration' = [recoveryObjectGeneration EXCEPT ![l][r1] = g1, ![l][r2] = g2]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                     anchorSyncPending, acknowledgements, abandoned, fatal, lineageEnabled, highWater >>

BrokenRecoverReplayedAnchor(l) ==
    /\ IF BrokenMode = "ReplayAnchor"
          THEN /\ durableAnchor[l] \in PositiveGenerations
               /\ manifestIdentity[l][durableAnchor[l]].manifestDigest = OtherDigest
               /\ durableAnchorIdentity[l].manifestDigest = durableAnchor[l]
          ELSE /\ BrokenMode = "CrossLineage"
               /\ durableAnchorIdentity[l].lineage # l
    /\ recovered' = [recovered EXCEPT ![l] = durableAnchor[l]]
    /\ recoveryObjectGeneration' = [recoveryObjectGeneration EXCEPT ![l] = [r \in Ranks |-> durableAnchor[l]]]
    /\ UNCHANGED << authority, authorityEpoch, writerAlive, attemptOwner, attemptGeneration,
                     attemptPredecessor, objectPhase, objectContent, manifestPhase,
                     manifestContent, manifestPredecessor, manifestIdentity, fingerprintOK,
                     liveAnchor, liveAnchorIdentity, durableAnchor, durableAnchorIdentity,
                     anchorSyncPending, acknowledgements, abandoned, fatal, lineageEnabled, highWater >>

Next ==
    \/ \E w \in Writers, l \in Lineages, g \in PositiveGenerations : BeginAttempt(w, l, g)
    \/ \E l \in Lineages, g \in PositiveGenerations, r \in Ranks : CreateObject(l, g, r)
    \/ \E l \in Lineages, g \in PositiveGenerations, r \in Ranks : AdvanceObject(l, g, r, "Created", "Written")
    \/ \E l \in Lineages, g \in PositiveGenerations, r \in Ranks : AdvanceObject(l, g, r, "Written", "Verified")
    \/ \E l \in Lineages, g \in PositiveGenerations, r \in Ranks : AdvanceObject(l, g, r, "Verified", "FileDurable")
    \/ \E l \in Lineages, g \in PositiveGenerations, r \in Ranks : PublishObjectNoReplace(l, g, r)
    \/ \E l \in Lineages, g \in PositiveGenerations, r \in Ranks, c \in {"Expected", "Other"} : InjectExistingObject(l, g, r, c)
    \/ \E l \in Lineages, g \in PositiveGenerations, r \in Ranks : VerifyExistingObject(l, g, r)
    \/ \E l \in Lineages, g \in PositiveGenerations, r \in Ranks : RejectObjectCollision(l, g, r)
    \/ \E l \in Lineages, g \in PositiveGenerations, r \in Ranks : AdvanceObject(l, g, r, "Named", "DirDurable")
    \/ \E l \in Lineages, g \in PositiveGenerations : WriteManifest(l, g)
    \/ \E l \in Lineages, g \in PositiveGenerations : AdvanceManifest(l, g, "Written", "Verified")
    \/ \E l \in Lineages, g \in PositiveGenerations : AdvanceManifest(l, g, "Verified", "FileDurable")
    \/ \E l \in Lineages, g \in PositiveGenerations : PublishManifestNoReplace(l, g)
    \/ \E l \in Lineages, g \in PositiveGenerations, c \in {"Expected", "Other"} : InjectExistingManifest(l, g, c)
    \/ \E l \in Lineages, g \in PositiveGenerations : VerifyExistingManifest(l, g)
    \/ \E l \in Lineages, g \in PositiveGenerations : RejectManifestCollision(l, g)
    \/ \E l \in Lineages, g \in PositiveGenerations : AdvanceManifest(l, g, "Named", "DirDurable")
    \/ \E l \in Lineages : AtomicReplaceAnchor(l)
    \/ \E l \in Lineages : SyncAnchor(l)
    \/ \E l \in Lineages, g \in PositiveGenerations : AckDurability(l, g)
    \/ \E l \in Lineages : AbandonGeneration(l)
    \/ \E l \in Lineages : CrashBeforeAnchorSyncOld(l)
    \/ \E l \in Lineages : CrashBeforeAnchorSyncNew(l)
    \/ CrashAuthority
    \/ \E w \in Writers : RestartWriter(w)
    \/ \E w \in Writers : TransferAuthority(w)
    \/ \E l \in Lineages, g \in Generations, r \in Ranks, p \in {"Corrupt", "Missing"} : CorruptObject(l, g, r, p)
    \/ \E l \in Lineages, g \in Generations, p \in {"Corrupt", "Missing"} : CorruptManifest(l, g, p)
    \/ \E l \in Lineages, g \in PositiveGenerations : StaleFingerprint(l, g)
    \/ \E l \in Lineages, g \in PositiveGenerations, f \in {"Digest", "Predecessor"} : MutateManifestIdentity(l, g, f)
    \/ \E target \in Lineages, source \in Lineages : ReplayCrossLineageAnchor(target, source)
    \/ \E l \in Lineages : RecoverSelected(l)
    \/ \E l \in Lineages : RejectSelected(l)
    \/ \E l \in Lineages : RecomputeAfterMiss(l)
    \/ \E l \in Lineages, g \in PositiveGenerations : BrokenRecoverNewest(l, g)
    \/ \E l \in Lineages, r1 \in Ranks, r2 \in Ranks, g1 \in Generations, g2 \in Generations : BrokenMixedRecovery(l, r1, r2, g1, g2)
    \/ \E l \in Lineages : BrokenRecoverReplayedAnchor(l)

Spec == Init /\ [][Next]_vars

IdentityType ==
    [lineage: Lineages, generation: Generations \cup {NoGeneration},
     manifestDigest: Generations \cup {NoGeneration, OtherDigest},
     predecessorDigest: Generations \cup {NoGeneration, OtherDigest},
     policyEpoch: {0}, keyGeneration: {0}, authorityEpoch: 0..MaxAuthorityEpoch]

TypeOK ==
    /\ authority \in Writers /\ authorityEpoch \in 0..MaxAuthorityEpoch
    /\ writerAlive \in [Writers -> BOOLEAN]
    /\ attemptOwner \in [Lineages -> (Writers \cup {NoWriter})]
    /\ attemptGeneration \in [Lineages -> Generations]
    /\ attemptPredecessor \in [Lineages -> (Generations \cup {NoGeneration})]
    /\ objectPhase \in [Lineages -> [Generations -> [Ranks -> ObjectPhases]]]
    /\ objectContent \in [Lineages -> [Generations -> [Ranks -> Contents]]]
    /\ manifestPhase \in [Lineages -> [Generations -> ManifestPhases]]
    /\ manifestContent \in [Lineages -> [Generations -> Contents]]
    /\ manifestPredecessor \in [Lineages -> [Generations -> (Generations \cup {NoGeneration})]]
    /\ manifestIdentity \in [Lineages -> [Generations -> IdentityType]]
    /\ fingerprintOK \in [Lineages -> [Generations -> BOOLEAN]]
    /\ liveAnchor \in [Lineages -> Generations]
    /\ liveAnchorIdentity \in [Lineages -> IdentityType]
    /\ durableAnchor \in [Lineages -> Generations]
    /\ durableAnchorIdentity \in [Lineages -> IdentityType]
    /\ anchorSyncPending \in [Lineages -> BOOLEAN]
    /\ acknowledgements \in [Lineages -> SUBSET Generations]
    /\ abandoned \in [Lineages -> SUBSET Generations]
    /\ fatal \in [Lineages -> BOOLEAN]
    /\ recovered \in [Lineages -> (Generations \cup {NoGeneration, Miss})]
    /\ recoveryObjectGeneration \in [Lineages -> [Ranks -> (Generations \cup {NoGeneration})]]
    /\ lineageEnabled \in [Lineages -> BOOLEAN]
    /\ highWater \in [Lineages -> Generations]

PublicationPrerequisites ==
    \A l \in Lineages : anchorSyncPending[l] =>
        /\ attemptOwner[l] = authority /\ writerAlive[authority]
        /\ liveAnchor[l] = attemptGeneration[l]
        /\ attemptPredecessor[l] = durableAnchor[l]
        /\ liveAnchor[l] = durableAnchor[l] + 1

NoAckBeforeVisibility == \A l \in Lineages : \A g \in acknowledgements[l] : g <= highWater[l]

RecoveryUsesExactAnchor ==
    \A l \in Lineages : recovered[l] \in Generations =>
        /\ recovered[l] = durableAnchor[l]
        /\ DurableAnchorValid(l)

NoMixedGenerationRecovery ==
    \A l \in Lineages : recovered[l] \in Generations =>
        \A r \in Ranks : recoveryObjectGeneration[l][r] = recovered[l]

AuthorityFencing ==
    \A l \in Lineages : attemptOwner[l] # NoWriter =>
        /\ attemptOwner[l] = authority /\ writerAlive[authority]

MonotonicProtectedAnchor == \A l \in Lineages : durableAnchor[l] = highWater[l]

LineageLocality ==
    /\ DOMAIN durableAnchor = Lineages
    /\ DOMAIN durableAnchorIdentity = Lineages
    /\ DOMAIN recovered = Lineages

Safety == TypeOK /\ PublicationPrerequisites /\ NoAckBeforeVisibility /\
          RecoveryUsesExactAnchor /\ NoMixedGenerationRecovery /\
          AuthorityFencing /\ MonotonicProtectedAnchor /\ LineageLocality

FullyPrepared(l, g) == AllObjectsDurable(l, g) /\ ManifestDurable(l, g)
Terminated(l, g) == durableAnchor[l] = g \/ g \in abandoned[l]
FullyPreparedTerminates ==
    \A l \in Lineages, g \in PositiveGenerations : [](FullyPrepared(l, g) => <>Terminated(l, g))

LiveSpec == Spec
    /\ (\A l \in Lineages : WF_vars(AtomicReplaceAnchor(l)))
    /\ (\A l \in Lineages : WF_vars(SyncAnchor(l)))
    /\ (\A l \in Lineages : WF_vars(AbandonGeneration(l)))

=============================================================================
