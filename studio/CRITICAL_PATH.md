# Critical Path

- Current blocker: PR #107 already owns the m8 module guard while this verified m8 slice is open.
- Next step: resolve the existing m8 PR queue, then merge this sample/portability slice.
- Likely next blocker: components need an ownership and serialization model that does not turn `m5` into a hidden manager.
