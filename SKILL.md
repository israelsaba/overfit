---
name: overfit-cli
description: Uses the overfit C CLI to inspect Git changes, generate Conventional Commit messages and pull request text with a configured AI model, then safely commit, push, and create GitHub pull requests. Use when a user asks to run overfit commit, overfit pr, or replace manual git add/commit/push and gh pr create workflows.
license: MIT
compatibility: Requires a POSIX environment with a C compiler, git, curl, and GitHub CLI (gh) for pull requests.
---

# Overfit CLI

Use `overfit commit` for a repository change that should be staged, committed with an AI-generated Conventional Commit message, and pushed.

Use `overfit main` to check out `main` and fast-forward it from its configured remote.

Use `overfit pr` to optionally commit pending changes, push the current branch, and create a GitHub pull request with AI-generated title and body. The command waits five seconds before the automatic commit. Press any key during the countdown to skip it, press Enter to run it immediately, or pass `--skip-commit` or `-sc`.

If `.github/pull_request_template.md` or `.github/PULL_REQUEST_TEMPLATE.md` exists, its required headings and checklists are supplied to the PR-generation step.

Commands show explicit gray high-level Git and GitHub invocations by default, while subprocess output uses a slightly lighter gray in interactive terminals. Use `--quiet` or `-q` to suppress only command previews; AI-call notices, prompts, and loading output remain visible.

If a Git or GitHub command fails during `commit` or `pr`, Overfit asks the configured provider for a bounded diagnosis using command output, recent Git history, repository state, branch, remotes, working directory, and non-secret terminal metadata. Provider, setup, and AI-request failures are not recursively sent to the provider.

Before the first use, run `overfit setup`. Choose OpenAI, xAI, Google AI, Anthropic, or a custom OpenAI-compatible base URL. Let it discover text-capable models when possible, select a model, and enter the API key. macOS stores the key in Keychain; the fallback config file uses mode 0600.

The CLI invokes `git`, `curl`, and `gh` with argument arrays. It does not evaluate generated text as shell code. Confirm the diff before using commands that push or create a pull request.
