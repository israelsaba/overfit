---
name: overfit-cli
description: Uses the overfit C CLI to inspect Git changes, generate Conventional Commit messages and pull request text with a configured AI model, then safely commit, push, and create GitHub pull requests. Use when a user asks to run overfit commit, overfit pr, or replace manual git add/commit/push and gh pr create workflows.
license: MIT
compatibility: Requires a POSIX environment with a C compiler, git, curl, and GitHub CLI (gh) for pull requests.
---

# Overfit CLI

Use `overfit commit` for a repository change that should be staged, committed with an AI-generated Conventional Commit message, and pushed.

Use `overfit pr` to optionally commit pending changes, push the current branch, and create a GitHub pull request with AI-generated title and body. The command waits five seconds before the automatic commit. Press any key during the countdown to skip it, or pass `--skip-commit` or `-sc`.

Before the first use, run `overfit setup`. Review the configured endpoint and model. The API key is stored at `~/.config/overfit/config` with mode 0600.

The CLI invokes `git`, `curl`, and `gh` with argument arrays. It does not evaluate generated text as shell code. Confirm the diff before using commands that push or create a pull request.
