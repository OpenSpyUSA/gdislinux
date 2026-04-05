# Contributing to GDIS

This repository is in a practical maintenance phase. The codebase is useful and
worth keeping alive, but it still depends on a legacy GTK2/OpenGL stack. Treat
changes as engineering work on a living legacy application, not as a greenfield
rewrite.

## Current Priorities

1. Keep the project buildable on a modern Linux distribution.
2. Improve repository hygiene so contributors can clone, build, and test
   without local guesswork.
3. Add repeatable CI and basic contributor tooling.
4. Reduce hardcoded assumptions in the build scripts.
5. Isolate GUI and GL integration points to make a later toolkit migration less
   risky.

## Build Locally

On Ubuntu 24.04:

```bash
sudo apt-get update
sudo apt-get install -y build-essential pkg-config libgtk2.0-dev libgtkglext1-dev
./rebuild-ubuntu.sh
```

Optional helper tools can be installed with:

```bash
./install-optional-ubuntu.sh
```

Useful local checks:

```bash
./verify-optional-tools.sh
./run-example.sh methane
./run-example.sh adp1
```

## Scope Discipline

Please keep changes staged and reviewable.

- Good first changes:
  - build-script cleanup
  - CI
  - documentation
  - warning cleanup in isolated files
  - removal of hardcoded environment assumptions
  - adding helper scripts for contributors
- Higher-risk changes:
  - GTK API migration
  - OpenGL context replacement
  - rendering-path refactors
  - deep parser rewrites
  - cross-platform packaging changes

If you touch a higher-risk area, keep the patch narrow and document the intent.

## Repo Hygiene

- Do not commit generated `src/*.o`, `src/*.d`, or `src/makefile` build outputs.
- Prefer updating helper scripts and documentation before adding one-off local
  instructions to issues or PR comments.
- If you improve local build behavior, document the reason in the README or the
  modernization roadmap.

## Suggested Maintenance Roadmap

See [`docs/modernization-roadmap.md`](docs/modernization-roadmap.md) for the
current staged plan.

## Publishing A Fork

If you are maintaining your own public GitHub fork from this tree, use
[`docs/github-fork-guide.md`](docs/github-fork-guide.md) for the initial Git
bootstrap, first commit, and remote setup workflow.

## Pull Request Guidance

When opening a PR, include:

- what problem you are solving
- which platforms you tested
- how you built the project
- any example files you used to verify behavior
- whether the change is prep work for a larger GTK/OpenGL migration
