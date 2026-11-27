# Agent instructions

## Every PR updates three files

`CHANGELOG.md`, `README.md` (section "Differences from upstream") and `docs/lusofw.md` describe every change lusofw makes on top of upstream MeshCore. Before opening a PR, check all three against the change and update each one it affects, in the same PR. This covers new defaults, migrations, build flags, board fixes and upstream syncs. An upstream sync also bumps the base version in `README.md` and `docs/lusofw.md`.

Check every column of the README table, not only whether the feature appears. A scope of "listed boards" must change when a fix starts to apply to every board. If a file needs no change, say so in the PR description.

## Language

Write `CHANGELOG.md` entries in European Portuguese (pt-PT) with a formal register, matching the existing entries.

## Attribution

Do not add `Co-Authored-By` trailers or "Generated with" lines to commits or PR descriptions.
