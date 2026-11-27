# Agent instructions

Before taking any action in this repository, read every memory listed in `.claude/memory/MEMORY.md` and follow it. This is mandatory.

## Keep the upstream-differences docs current

`README.md` (section "Differences from upstream") and `docs/lusofw.md` describe every behaviour change lusofw makes on top of upstream MeshCore. Any change that adds, removes or alters one of those differences must update both files in the same PR. This includes new defaults, migrations, build flags, board fixes and upstream syncs. An upstream sync also bumps the base version in both files.

## Every PR updates three files

Before opening a PR, check `CHANGELOG.md`, `README.md` and `docs/lusofw.md` against the change, and update each one that it affects in the same PR. Check every column of the README table, not only whether the feature appears. For example, a scope of "listed boards" must change when a fix starts to apply to every board. If a file needs no change, say so in the PR description.

Write `CHANGELOG.md` entries in European Portuguese (pt-PT) with a formal register, matching the existing entries.
