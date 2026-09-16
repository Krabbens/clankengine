"""Verify claims are visible from a second linked worktree."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


tool = str(Path(sys.argv[1]).resolve())
repo = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="clank-claim-") as temp:
    worktree = Path(temp) / "agent"
    topic = f"worktree-test-{os.getpid()}"
    branch = f"a/m7-{topic}"
    subprocess.run(["git", "worktree", "add", str(worktree), "-b", branch, "HEAD"],
                   cwd=repo, check=True, capture_output=True, text=True)
    claimed_ok = False
    try:
        worktree_tool = str(worktree / "tools" / "clank-claim")
        missing_intent = subprocess.run(
            [worktree_tool, "claim", "m7", "--agent", "test-agent", "--topic", topic],
            cwd=worktree, capture_output=True, text=True)
        assert missing_intent.returncode != 0
        assert "requires --intent" in missing_intent.stderr
        claimed = subprocess.run(
            [worktree_tool, "claim", "m7", "--agent", "test-agent", "--topic", topic,
             "--intent", "verify linked worktree claim"],
            cwd=worktree, check=True, capture_output=True, text=True)
        assert "claimed m7" in claimed.stdout
        claimed_ok = True
        rows = json.loads(subprocess.run(
            [tool, "list", "--json"], cwd=repo, check=True, capture_output=True, text=True
        ).stdout)
        m7 = next(row for row in rows if row["module"] == "m7")
        assert m7["status"] == "fresh" and m7["agent"] == "test-agent"
    finally:
        if claimed_ok:
            subprocess.run([worktree_tool, "release", "m7", "--agent", "test-agent"],
                           cwd=worktree, check=False, capture_output=True, text=True)
        subprocess.run(["git", "worktree", "remove", "--force", str(worktree)],
                       cwd=repo, check=True, capture_output=True, text=True)
        subprocess.run(["git", "branch", "-D", branch], cwd=repo, check=False,
                       capture_output=True, text=True)

print("claim: worktree isolation PASS")
