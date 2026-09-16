# Risks

| Risk | Probability | Impact | Criticality | Early warning | Mitigation | Fallback | Status |
|---|---:|---:|---:|---|---|---|---|
| Scene API grows into a hidden manager | 3 | 5 | P0 | more lifecycle state appears in `m5` | keep free functions and value data | stop at transform graph | open |
| Hierarchy serialization breaks old dumps | 2 | 4 | P1 | existing goldens gain unexpected keys | make `parent` optional and preserve roots | bump schema explicitly | open |
| O(n²) parent lookup becomes a real bottleneck | 2 | 3 | P1 | bench exceeds frame budget on large scenes | add a measured index only after evidence | keep linear lookup for small scenes | open |
