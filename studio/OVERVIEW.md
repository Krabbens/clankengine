# Clankengine Studio

## Current direction

Build a tiny, agent-first game engine that approaches Unreal-like capability
through explicit, deterministic slices rather than a large editor framework.

## Current experiment

`m5-scene-hierarchy`: add stable parent-linked scene entities and deterministic
world-transform resolution.

## Last decision

Scene hierarchy is the first capability slice because it supports Actor-like
composition, prefab roots, and later component attachment without adding a
runtime manager or a new dependency.

## Open

The next decision is how to represent components and asset ownership while
keeping the public core API small and headless-observable.

## Evidence so far

The Windows build passes. The focused suite passes `m5-selftest`, headless dump,
replay, and minimal sample. Full CTest still has pre-existing Windows harness
failures around `.exe` names and direct script execution.

## Next action

Add one headless sample assertion that consumes resolved hierarchy data, then
separate the Windows CTest harness repair into its own m8 sprint.
