# Critical Path

- Current blocker: the Windows CTest harness is red even though the hierarchy sample is green.
- Next step: repair Windows executable naming, temp paths, and Python tool launching in an isolated m8 slice.
- Likely next blocker: components need an ownership and serialization model that does not turn `m5` into a hidden manager.
