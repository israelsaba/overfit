---
type: knowledge_bundle_index
title: "Overfit: AI commit message and pull request generator CLI"
description: Overfit is a small C CLI and Agent Skill that writes Conventional Commit messages and GitHub pull request titles and bodies with OpenAI, Gemini, xAI, Anthropic, or any OpenAI-compatible model, then commits, pushes, and runs gh pr create.
tags: [agent-skill, ai-commit, commit-message-generator, pull-request-generator, conventional-commits, git, github-cli, c, cli, opencode, claude-code, codex]
timestamp: 2026-11-02T00:00:00Z
---

# Overfit: AI commit message and pull request generator CLI

Overfit is an AI commit message generator and GitHub pull request generator written in C. One command replaces this routine:

```text
git add --all && git commit -m "..." && git push
gh pr create --title "..." --body "..."
```

Overfit reads your diff, asks the model you configured for a [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) message or a PR title and body, and passes the result to `git` and `gh` as process arguments. Generated text is never run through a shell. It also ships as an Agent Skill, so OpenCode, Claude Code, Codex, and other Agent Skills clients can call it for you.

## At a glance

| Question | Answer |
| --- | --- |
| What it does | Stages, commits with an AI-written Conventional Commit, pushes, and opens a GitHub PR with an AI-written title and body |
| Commands | `overfit commit`, `overfit pr`, `overfit main`, `overfit setup` |
| AI providers | OpenAI, Google Gemini, xAI Grok, Anthropic Claude, or a custom OpenAI-compatible base URL |
| Runtime | One compiled C binary. Calls `git`, `curl`, and `gh`. No Node.js or Python runtime |
| Platforms | macOS and Linux |
| Credentials | macOS Keychain, or a mode 0600 config file elsewhere |
| Agent support | `SKILL.md` for OpenCode, Claude Code, Codex, Hermes, and other Agent Skills clients |
| License | MIT |

## Quick start

With [Homebrew](https://brew.sh) on macOS or Linux:

```sh
brew tap israelsaba/overfit https://github.com/israelsaba/overfit-homebrew
brew install israelsaba/overfit/overfit
overfit setup       # pick a provider and model, enter the API key once
overfit pr          # commit, push, and open a pull request
```

From source:

```sh
git clone https://github.com/israelsaba/overfit.git
cd overfit
./install.sh        # compiles locally into ~/.local/bin/overfit
overfit setup
```

To let a coding agent use it, copy this directory to your agent's skill folder, for example `~/.config/opencode/skills/overfit-cli/` or `~/.claude/skills/overfit-cli/`, and start a new session. See [Agent Skill](#agent-skill).

## Terminology

Some command names use common developer shorthand rather than perfectly precise terminology. For example, `main` commonly means the repository's primary branch, and `commit` commonly means the broader stage, commit, and push workflow in casual developer conversation. These names are deliberate choices based on terms that are widely understood when communicating loosely, even among developers. This will remain a recurring Overfit design theme: prefer familiar, memorable language while documenting the exact behavior underneath.

## Install

### Homebrew tap

The Homebrew formula lives in the [israelsaba/overfit-homebrew](https://github.com/israelsaba/overfit-homebrew) tap and builds the pinned, SHA-256-checked release tarball. The tap repository has no `homebrew-` prefix, so `brew tap` takes the URL as a second argument:

```sh
brew tap israelsaba/overfit https://github.com/israelsaba/overfit-homebrew
brew install israelsaba/overfit/overfit
brew upgrade israelsaba/overfit/overfit   # later, after brew update
```

In a `Brewfile`:

```ruby
tap "israelsaba/overfit", "https://github.com/israelsaba/overfit-homebrew"
brew "israelsaba/overfit/overfit"
```

If you installed from the old `israelsaba/tap` name, run `brew uninstall overfit` and `brew untap israelsaba/tap` first. Read [`Formula/overfit.rb`](https://github.com/israelsaba/overfit-homebrew/blob/main/Formula/overfit.rb) before installing, because tap formulae run with your user's privileges.

### From source

Review `src/overfit.c` and `install.sh`, then run the source commands in [Quick start](#quick-start). Pin a release tag such as `git checkout v0.1.0` when you want a reviewed, fixed version.

The installer is local-only: it compiles with the system C compiler through `make install` and downloads nothing. Ensure `~/.local/bin` is on `PATH`. Requirements: a C11 compiler, `make`, `git`, `curl`, and an authenticated [GitHub CLI](https://cli.github.com/) for pull requests.

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

If an open pull request already exists for the current branch, Overfit detects it before calling the PR-generation AI, pushes any new commit to that branch, and reports the existing PR instead of attempting a duplicate.

When `.github/pull_request_template.md` or `.github/PULL_REQUEST_TEMPLATE.md` exists, its first 20,000 bytes are included in the PR-generation context so required sections and checklists can be filled.

Commit and PR generation include recent history, the current and upstream branches, repository overview and contribution guidance when present, change statistics, tracked diffs, and bounded contents of untracked files. PR generation also compares `HEAD` with `origin`'s default branch, then falls back to `main` or `master`, so it can describe the committed branch changes after the automatic commit.

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

`SKILL.md` is the portable Agent Skills entry point. The skill is named `overfit-cli`; the repository is `overfit`. Install the `overfit` binary first, then copy this directory into a skill folder and start a new agent session:

| Agent | Skill directory |
| --- | --- |
| OpenCode | `~/.config/opencode/skills/overfit-cli/` or `.opencode/skills/overfit-cli/` |
| Claude Code | `~/.claude/skills/overfit-cli/` or `.claude/skills/overfit-cli/` |
| Codex | `.agents/skills/overfit-cli/` in the repository |
| Other Agent Skills clients | The client's documented skill directory |

```sh
mkdir -p ~/.config/opencode/skills/overfit-cli
cp SKILL.md ~/.config/opencode/skills/overfit-cli/
```

Review `SKILL.md` before installing it. An agent follows its instructions with your session's permissions, and `overfit commit` and `overfit pr` push to your remote.

## Verification

```sh
make
./overfit --help
make clean
```

The repository's release checks also run compiler warnings, shell checks, secret scanning, and Semgrep when available. Static analysis cannot verify the behavior of a remote AI service or GitHub permissions.

## Use cases

- Write Conventional Commit messages (`feat:`, `fix:`, `docs:`, `refactor:`) from the working-tree diff without opening an editor.
- Open a GitHub pull request whose title and body describe the whole branch, filled in against your `.github/pull_request_template.md`.
- Push follow-up commits to a branch that already has an open PR without creating a duplicate.
- Return to an up-to-date `main` after a merge with `overfit main`.
- Let a coding agent finish its task with a reviewed commit and PR through the `overfit-cli` skill.
- Get an AI explanation when `git push`, `git pull --ff-only`, or `gh pr create` fails.

## FAQ

### How do I generate a commit message with AI?

Run `overfit setup` once, then `overfit commit` in the repository. Overfit stages all changes with `git add --all`, sends the diff and recent history to your model, commits with the returned Conventional Commit message, and runs `git push`.

### How do I create a GitHub pull request description with AI?

Run `overfit pr`. It commits pending changes after a five-second countdown, pushes the branch, writes a PR title and body from the comparison with the default branch, and calls `gh pr create --title ... --body ...`. Use `overfit pr --skip-commit` to open a PR from commits you already made.

### Which AI models does Overfit support?

`overfit setup` has presets for OpenAI, Google Gemini through its OpenAI-compatible endpoint, xAI, and Anthropic Claude through the native Messages API. Any other base URL that implements `/chat/completions` can be entered as a custom provider.

### How much does each commit or PR cost?

It depends on the model and the diff size. Overfit sends at most about 240,000 bytes of repository context, roughly 60,000 tokens. With a model priced at $0.30 per million input tokens and $2.50 per million output tokens, a request at that ceiling costs about two cents, and a typical small diff costs a fraction of a cent. Dozens of commits and PRs stay under one dollar on a small model. Check your provider's current pricing.

### Does Overfit ask before committing or pushing?

No. `overfit commit` stages everything, commits, and pushes without a confirmation prompt. `overfit pr` gives a five-second window to skip the automatic commit. Review `git status` and `git diff` first, and use it in repositories where pushing your branch is safe.

### Is my code sent to the AI provider?

Yes. Diffs, bounded contents of untracked files, recent history, branch names, and repository guidance files go to the endpoint you configure. Point it at a provider you trust with that code.

### What does Overfit cover beyond a commit message?

The whole path from working tree to open pull request: stage, commit, push, and `gh pr create`, with PR template filling, existing-PR detection, and failure diagnosis. It is a single C binary with no package-manager runtime, and it doubles as an Agent Skill.

## Sources

- [Agent Skills specification](https://agentskills.io/specification), accessed 2026-10-16.
- [Commitizen documentation](https://commitizen-tools.github.io/commitizen/), accessed 2026-10-16. Commitizen defaults to Conventional Commits.
- [Conventional Commits specification](https://www.conventionalcommits.org/en/v1.0.0/), version 1.0.0.
- [GitHub CLI `gh pr create` manual](https://cli.github.com/manual/gh_pr_create), accessed 2026-10-16.
- [OpenAI API compatibility](https://platform.openai.com/docs/api-reference/models), accessed 2026-10-16.
- [xAI API documentation](https://docs.x.ai/docs), accessed 2026-10-16.
- [Google Gemini OpenAI compatibility](https://ai.google.dev/gemini-api/docs/openai), accessed 2026-10-16.
- [Anthropic Messages API](https://docs.anthropic.com/en/api/messages), accessed 2026-10-16.
- [Homebrew taps](https://docs.brew.sh/Taps), accessed 2026-11-02.
- [OpenCode skills](https://opencode.ai/docs/skills/), accessed 2026-11-02.
- [Claude Code skills](https://docs.claude.com/en/docs/claude-code/skills), accessed 2026-11-02.
- [Codex repository skills in `.agents/skills/`](https://developers.openai.com/blog/skills-agents-sdk), accessed 2026-11-02.
- [Gemini API pricing](https://ai.google.dev/gemini-api/docs/pricing), accessed 2026-11-02, for the cost example.

## Contributing

Pull requests are welcome. A useful PR includes a focused change, a reproducible test command, and security impact notes when process execution or credential handling changes. File issues for reproducible bugs, unclear instructions, installation problems, compatibility reports, and focused proposals. Include the version or commit, agent and CLI versions, operating system, expected behavior, actual behavior, and a minimal example. Never post secrets or private data publicly. Stable releases are tagged as `vMAJOR.MINOR.PATCH` and published through GitHub Releases.

## License

MIT. See [LICENSE](LICENSE).
