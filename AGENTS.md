# Agent instructions

Before taking any action in this repository, read every memory listed in `.claude/memory/MEMORY.md` and follow it. This is mandatory.

## Keep the upstream-differences docs current

`README.md` (section "Differences from upstream") and `docs/lusofw.md` describe every behaviour change lusofw makes on top of upstream MeshCore. Any change that adds, removes or alters one of those differences must update both files in the same PR. This includes new defaults, migrations, build flags, board fixes and upstream syncs. An upstream sync also bumps the base version in both files.
