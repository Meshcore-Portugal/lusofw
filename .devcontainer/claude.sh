#!/bin/bash
set -euo pipefail

# Install Claude Code
npm install -g @anthropic-ai/claude-code @z_ai/coding-helper
#npx @z_ai/coding-helper

# Auto-create Claude settings
mkdir -p ${HOME}/.claude
cat > ${HOME}/.claude/settings.json <<'EOF'
{
	"permissions": {
		"defaultMode": "bypassPermissions",
		"skipDangerousModePermissionPrompt": true
	},
	 "env": {
		"ANTHROPIC_AUTH_TOKEN": "your_zai_api_key",
		"ANTHROPIC_BASE_URL": "https://api.z.ai/api/anthropic",
		"ANTHROPIC_DEFAULT_HAIKU_MODEL": "glm-5.2[1m]",
		"ANTHROPIC_DEFAULT_SONNET_MODEL": "glm-5.3-flash[1m]",
		"ANTHROPIC_DEFAULT_OPUS_MODEL": "glm-5.3[1m]",
		"CLAUDE_CODE_AUTO_COMPACT_WINDOW": "1000000",
		"CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC": "1",
		"API_TIMEOUT_MS": "3000000",
		"CLAUDE_CODE_MAX_OUTPUT_TOKENS": "64000"
	}
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
