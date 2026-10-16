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

## Terminology

Some command names use common developer shorthand rather than perfectly precise terminology. For example, `main` commonly means the repository's primary branch, and `commit` commonly means the broader stage, commit, and push workflow in casual developer conversation. These names are deliberate choices based on terms that are widely understood when communicating loosely, even among developers. This will remain a recurring Overfit design theme: prefer familiar, memorable language while documenting the exact behavior underneath.

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
overfit main
```

By default, Overfit displays high-level commands and subprocess output using explicit ANSI grayscale levels, with commands slightly darker than output. These styles are used only in interactive terminals and avoid the terminal's default white foreground. Use the common quiet flags `--quiet` or `-q` to omit only command previews, for example `overfit -q pr` or `overfit pr --quiet`. AI-call notices, prompts, and loading indicators remain visible.

`overfit commit` stages all pending changes, creates one AI-generated Conventional Commit, and pushes with `git push`.

`overfit main` checks out the local `main` branch and updates it with `git pull --ff-only`. Git refuses the checkout if local changes would be overwritten.

`overfit pr` detects pending changes and waits five seconds before committing. Press any key during the countdown to skip the automatic commit; press Enter to run it immediately. You can also use `--skip-commit` / `-sc`. It then pushes the current branch and runs `gh pr create` with AI-generated `--title` and `--body` arguments.

When `.github/pull_request_template.md` or `.github/PULL_REQUEST_TEMPLATE.md` exists, its first 20,000 bytes are included in the PR-generation context so required sections and checklists can be filled.

When a Git or GitHub command fails during `overfit commit`, `overfit pr`, or `overfit main`, Overfit captures the command output and sends a bounded diagnostic context to the configured AI when setup is available. The context includes the full available Git history when it fits within a 120,000-byte history section, status and diff information, branch, remote names, working directory, terminal metadata, and the failed command output. AI request, provider, and setup errors are never sent back to the AI for analysis.

## Configuration

`overfit setup` offers presets for OpenAI (`https://api.openai.com/v1`), xAI (`https://api.x.ai/v1`), Google AI (`https://generativelanguage.googleapis.com/v1beta/openai`), and Anthropic (`https://api.anthropic.com/v1`). It also accepts any custom OpenAI-compatible base URL. After the provider is selected, enter the API key once so authenticated model discovery can run. It filters obvious non-text models and lets you select a model. If discovery is unavailable, enter the model ID manually.

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
