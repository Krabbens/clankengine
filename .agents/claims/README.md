# Claims

One file per module: `m0.json` .. `m8.json`. See `AGENTS.md` §5.

* Never edit another agent's claim by hand. Use `tools/clank-claim`.
* `list` shows fresh vs stale (`now - heartbeat_utc > ttl_min` = stale).
* Empty directory = all modules free.
* Do not commit stale claims. CI cron labels them.
