---
type: knowledge_bundle_index
title: Overfit CLI
description: A small C CLI that uses a configured AI model to create Conventional Commits and GitHub pull requests from repository changes.
tags: [agent-skill, cli, c, git, github, conventional-commits]
timestamp: 2026-10-16T00:00:00Z
---

# Overfit CLI

Overfit is a local C CLI and Agent Skill for the repetitive Git workflow:

```text
git add --all && git commit ... && git push
gh pr create ...
```

It inspects the repository, asks a configured OpenAI-compatible chat API for a Conventional Commit message or PR metadata, and passes generated values as process arguments. It does not execute generated text as a shell command.

## Install

Review the source, then install locally on macOS or Linux:

```sh
git clone https://github.com/israelsaba/overfit.git
cd overfit
./install.sh
overfit setup
```

The installer is local-only and compiles with the system C compiler. Ensure `~/.local/bin` is on `PATH`.

## Commands

```sh
overfit commit
overfit pr
overfit pr --skip-commit
overfit pr -sc
```

`overfit commit` stages all pending changes, creates one AI-generated Conventional Commit, and pushes with `git push`.

`overfit pr` detects pending changes and waits five seconds before committing. Press any key during the countdown to skip the automatic commit, or use `--skip-commit` / `-sc`. It then pushes the current branch and runs `gh pr create` with AI-generated `--title` and `--body` arguments.

## Configuration

`overfit setup` offers presets for OpenAI (`https://api.openai.com/v1`), xAI (`https://api.x.ai/v1`), Google AI (`https://generativelanguage.googleapis.com/v1beta/openai`), and Anthropic (`https://api.anthropic.com/v1`). It also accepts any custom OpenAI-compatible base URL. It attempts to fetch the provider's model list, filters obvious non-text models, and lets you select a model. If discovery is unavailable, enter the model ID manually.

```sh
overfit setup
```

The OpenAI-compatible providers use `/chat/completions` and `/models`. Anthropic uses its native `/messages` API and `/models` endpoint. The API key is requested without terminal echo. On macOS it is stored in Keychain under `overfit-api-key`; on other systems it is stored in `~/.config/overfit/config` with mode 0600 so it survives restarts.

## Security

- Generated commit and PR text is passed through `execvp` argument vectors, never through a shell.
- macOS API credentials use Keychain; the fallback config file is written with 0600 permissions.
- `overfit commit` and `overfit pr` can push remote changes. Review `git diff` before running them.
- `gh` authentication and repository permissions remain controlled by GitHub CLI.
- Do not commit the local configuration file or place secrets in this repository.

## Agent Skill

`SKILL.md` is the portable Agent Skills entry point. For OpenCode, copy this directory to `~/.config/opencode/skills/overfit-cli/`, then start a new session. The same `SKILL.md` directory can be copied to the skill directory supported by another Agent Skills-compatible client.

## Verification

```sh
make
./overfit --help
make clean
```

The repository's release checks also run compiler warnings, shell checks, secret scanning, and Semgrep when available. Static analysis cannot verify the behavior of a remote AI service or GitHub permissions.

## Sources

- [Agent Skills specification](https://agentskills.io/specification), accessed 2026-10-16.
- [Commitizen documentation](https://commitizen-tools.github.io/commitizen/), accessed 2026-10-16. Commitizen defaults to Conventional Commits.
- [Conventional Commits specification](https://www.conventionalcommits.org/en/v1.0.0/), version 1.0.0.
- [GitHub CLI `gh pr create` manual](https://cli.github.com/manual/gh_pr_create), accessed 2026-10-16.
- [OpenAI API compatibility](https://platform.openai.com/docs/api-reference/models), accessed 2026-10-16.
- [xAI API documentation](https://docs.x.ai/docs), accessed 2026-10-16.
- [Google Gemini OpenAI compatibility](https://ai.google.dev/gemini-api/docs/openai), accessed 2026-10-16.
- [Anthropic Messages API](https://docs.anthropic.com/en/api/messages), accessed 2026-10-16.

## Contributing

Pull requests are welcome. A useful PR includes a focused change, a reproducible test command, and security impact notes when process execution or credential handling changes. File issues for reproducible bugs, unclear instructions, installation problems, compatibility reports, and focused proposals. Include the version or commit, agent and CLI versions, operating system, expected behavior, actual behavior, and a minimal example. Never post secrets or private data publicly. Stable releases are tagged as `vMAJOR.MINOR.PATCH` and published through GitHub Releases.

## License

MIT. See [LICENSE](LICENSE).
