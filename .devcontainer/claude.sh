#!/bin/bash
set -euo pipefail

# Install Claude Code
npm install -g @anthropic-ai/claude-code @z_ai/coding-helper
#npx @z_ai/coding-helper

# Auto-create Claude settings
mkdir -p ${HOME}/.claude
cat > ${HOME}/.claude/settings.json <<'EOF'
{
  "env": {
    "ANTHROPIC_DEFAULT_HAIKU_MODEL": "cheap-coding[1m]",
    "ANTHROPIC_DEFAULT_OPUS_MODEL": "premium-coding[1m]",
    "ANTHROPIC_DEFAULT_SONNET_MODEL": "normal-coding[1m]",
    "CLAUDE_CODE_AUTO_COMPACT_WINDOW": "1000000",
    "CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC": "1",
    "CLAUDE_CODE_MAX_OUTPUT_TOKENS": "64000",
    "CLAUDE_MODEL": "opusplan",
    "DISABLE_ERROR_REPORTING": "1",
    "DISABLE_TELEMETRY": "1"
  },
  "permissions": {
    "defaultMode": "bypassPermissions",
    "allow": [
			"Bash(git grep *)",
			"Bash(pio run)",
			"Bash(pio run *)",
			"Bash(pio device monitor *)",
			"Bash(pio project config *)",
			"Bash(pio pkg list *)"
    ]
  },
  "theme": "dark",
  "skipDangerousModePermissionPrompt": true,
  "effortLevel": "high",
  "model": "opusplan"
}
EOF

# Persist agent memory in the repo: the per-project memory dir
# (~/.claude/projects/<workspace-path-slug>/memory) is symlinked to the
# git-tracked .claude/memory so memories survive container rebuilds.
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MEMORY_SRC="${REPO_ROOT}/.claude/memory"
MEMORY_DST="${HOME}/.claude/projects/$(printf '%s' "${REPO_ROOT}" | tr '/' '-')/memory"
mkdir -p "${MEMORY_SRC}" "$(dirname "${MEMORY_DST}")"
if [ "$(readlink "${MEMORY_DST}")" != "${MEMORY_SRC}" ]; then
	rm -rf "${MEMORY_DST}"
	ln -sn "${MEMORY_SRC}" "${MEMORY_DST}"
fi
