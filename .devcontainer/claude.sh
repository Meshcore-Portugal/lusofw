#!/bin/bash
set -euo pipefail

# Install Claude Code
npm install -g -allow-scripts=@anthropic-ai/claude-code @anthropic-ai/claude-code

# Auto-create Claude settings
mkdir -p ${HOME}/.claude
cat > ${HOME}/.claude/settings.json <<'EOF'
{
  "env": {
    "ANTHROPIC_DEFAULT_FABLE_MODEL": "flagship",
    "ANTHROPIC_DEFAULT_OPUS_MODEL": "pro",
    "ANTHROPIC_DEFAULT_SONNET_MODEL": "flash",
    "ANTHROPIC_DEFAULT_HAIKU_MODEL": "flash-lite",
    "_CLAUDE_CODE_AUTO_COMPACT_WINDOW": "150000",
    "_CLAUDE_CODE_MAX_OUTPUT_TOKENS": "16384",
    "CLAUDE_MODEL": "flash",
    "DISABLE_ERROR_REPORTING": "1",
    "DISABLE_TELEMETRY": "1"
  },
  "permissions": {
    "defaultMode": "bypassPermissions"
  },
  "effortLevel": "medium",
  "skipDangerousModePermissionPrompt": true,
  "theme": "dark"
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

claude plugin install code-review-anthropics@anthropic-plugin-directory --scope user
claude plugin install code-simplifier@anthropic-plugin-directory --scope user
claude plugin install frontend-design@anthropic-plugin-directory --scope user
claude plugin install ralph-loop@anthropic-plugin-directory --scope user
claude plugin install security-guidance@anthropic-plugin-directory --scope user
claude plugin install skill-creator@anthropic-plugin-directory --scope user
claude plugin install superpowers@anthropic-plugin-directory --scope user
