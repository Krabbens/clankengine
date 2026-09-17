# Critical Path

- Current blocker: PR #107 already owns the m8 module guard while this verified m8 slice is open.
- Next step: close or update the older m8 PR through its owner, then rerun guard and merge #109.
- Verified foundation: m1 live input, m2 checked asset handles, m3 QueryAabb, m4 SetVelocity, m5 ComponentStore, m6 named actions, and m7 Windows shot harness are merged.
- Likely next slice: explicit Actor lifecycle; keep it small and deterministic without turning `m5` into a hidden manager.
