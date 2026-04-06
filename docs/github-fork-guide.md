# Publishing Your Fork on GitHub

This repository is currently a practical maintenance fork of the upstream GDIS
codebase. The goal is to preserve a buildable baseline, document the legacy
stack honestly, and make staged modernization possible.

This guide is intentionally simple. It is aimed at taking the current local
tree and turning it into your own GitHub repository cleanly.

## What This Folder Already Contains

The local tree already includes:

- build helpers such as `./rebuild-ubuntu.sh`
- example launch helpers such as `./run-example.sh`
- optional-tool helpers such as `./verify-optional-tools.sh`
- contributor documentation and a staged maintenance roadmap
- a current `bin/gdis` build and the patched source tree

The main thing missing for publishing is Git itself and a local repository
history.

## Recommended First Commit Shape

For a clean public starting point, use a single bootstrap commit first.

Suggested message:

```text
fork: bootstrap community-maintained GDIS revival
```

After that, split future work into narrower commits such as:

- `build: improve installer and Ubuntu rebuild flow`
- `docs: add maintenance roadmap and contributor notes`
- `port: add GTK/OpenGL compatibility wrappers`
- `port: replace deprecated GTK widget and image APIs`

## Install Git

If Git is not installed on the machine:

```bash
sudo apt-get update
sudo apt-get install -y git
```

## Initialize The Repository

Run these commands inside the project root:

```bash
cd /home/hym/Desktop/gdislinux
git init
git add .
git status
```

Before committing, make sure your identity is configured:

```bash
git config user.name "Your Name"
git config user.email "you@example.com"
```

Then create the initial commit:

```bash
git commit -m "fork: bootstrap community-maintained GDIS revival"
git branch -M main
```

## Create The GitHub Remote

Create an empty repository on GitHub first. Then connect this local tree to it.

SSH remote:

```bash
git remote add origin git@github.com:<your-user>/<your-repo>.git
```

HTTPS remote:

```bash
git remote add origin https://github.com/<your-user>/<your-repo>.git
```

Push the branch:

```bash
git push -u origin main
```

## Suggested Repository Naming

Any of these are reasonable:

- `gdis`
- `gdis-revival`
- `gdis-modernized`
- `gdislinux`

If your goal is to preserve continuity with the original project, `gdis` is the
cleanest choice. If you want to emphasize fork status, `gdis-revival` is a good
middle ground.

## What To Keep In The README

For a public fork, keep these points visible:

- original authorship remains credited
- the project is GPL-licensed
- the current supported baseline is still GTK2 plus `gtkglext`
- the near-term goal is maintainability, not a reckless rewrite
- the long-term goal is reducing dependence on obsolete GUI libraries

## Recommended Next Maintenance Steps

After the first push:

1. Remove any accidentally tracked build outputs if they were committed before
   `.gitignore` was in place.
2. Tag the first public state, for example `fork-bootstrap`.
3. Keep one issue or project note for the GTK/OpenGL migration plan.
4. Make the next technical milestone renderer parity on the GTK4 path while
   keeping GTK2 stable for users who need the legacy stack.

## License And Attribution

This codebase is distributed under the GPL terms already included in
[`LICENSE`](../LICENSE). A public fork should preserve:

- the existing license file
- copyright headers in source files
- explicit attribution to the original authors

## Practical Note About Upstream History

This directory came from a source archive, not from a cloned Git repository.
That means your first commit will be a fresh-root history unless you later
import or reconstruct upstream Git history manually. That is acceptable for a
maintenance fork as long as authorship and licensing remain clear.
