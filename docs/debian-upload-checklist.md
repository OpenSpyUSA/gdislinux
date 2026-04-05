# GDIS Debian Reintroduction Checklist

This checklist is for the renewed GTK4 branch in this repository.

## 1. Before pushing to GitHub

- Review the worktree and decide which local-only files should stay out of the public branch.
- Confirm the maintainer identity in [`debian/control`](../debian/control) and [`debian/changelog`](../debian/changelog).
- Replace the current `Vcs-Browser` and `Vcs-Git` fields in [`debian/control`](../debian/control) with your own GitHub fork URL after the fork exists.
- Keep the Debian packaging tied to the GTK4 path until the GTK2/`gtkglext` path is either removed or clearly marked legacy.
- Re-run packaging checks from a clean export:
  - `dpkg-buildpackage -S -us -uc`
  - `lintian --profile debian ../gdis_*_source.changes`

## 2. GitHub push checklist

- Create your fork of `https://github.com/arohl/gdis`.
- Add your fork as a Git remote, for example:
  - `git remote add fork git@github.com:YOUR_USER/gdis.git`
- Create a branch for the renewal and packaging work.
- Commit the packaging refresh separately from large GTK4 rendering changes if you want easier review.
- Push the branch and make sure the repository contains:
  - the renewed GTK4 source changes
  - the `debian/` packaging directory
  - this checklist and any verification scripts you want to keep public
- Update the `Homepage`, `Vcs-Browser`, and `Vcs-Git` fields only if the fork becomes the canonical packaging location.

## 3. Debian reintroduction checklist

- Check why `gdis` was removed from Debian and verify that the GTK4 renewal addresses the removal reason.
- Search WNPP to see whether there is already an active RFP/ITP/reintroduction effort for `gdis`.
- If there is no active bug, file a WNPP bug for reintroduction work.
- Once there is a real Debian bug number, add a changelog entry such as `Closes: #BUGNUMBER` in the upload that should close it.
- Build a source package from a clean tree, not from the live development workspace.
- Upload the signed source package to `mentors.debian.net`.
- File an RFS bug against the `sponsorship-requests` pseudo-package using the mentors template.
- Ask for sponsorship on `debian-mentors@lists.debian.org` and, if appropriate, relevant science/chemistry packaging teams.
- Be ready to respond to sponsor review, adjust packaging, and upload revised source packages to mentors repeatedly.

## 4. Technical checks before asking for a sponsor

- Build in a clean Debian-like environment if possible, not only on the local desktop.
- Verify the package still builds with:
  - `dpkg-buildpackage -us -uc -b`
  - `dpkg-buildpackage -S -us -uc`
- Run:
  - `lintian --profile debian path/to/*.changes`
- Confirm the binary package installs:
  - `/usr/bin/gdis`
  - `/usr/lib/gdis/gdis`
  - `/usr/share/applications/gdis.desktop`
  - `/usr/share/pixmaps/gdis.xpm`
- Test the GTK4 runtime against representative files:
  - molecule display and selection
  - diffraction
  - iso-surface
  - Qbox round-trip files if you plan to mention Qbox support

## 5. Debian-specific follow-up items still worth doing

- Replace the temporary icon choice if you want a better desktop icon than the current XPM fallback.
- Consider moving runtime data lookup out of the wrapper and into upstream path handling later.
- Consider cleaning up deprecated GTK4 API usage over time; it builds now, but warnings remain.
- If you adopt team maintenance later, keep your personal identity in `Maintainer` or move to a team address and add yourself in `Uploaders`, according to Debian practice.

## 6. Current status of this repository

- Debian packaging now targets GTK4 and does not rely on `gtkglext`.
- The package builds successfully as both source and binary from a clean exported snapshot.
- `lintian --profile debian` on the clean source package is currently clean.
- The binary package only had the expected initial-upload warning before a real Debian bug closure is added.

## References

- Debian Developer's Reference: https://www.debian.org/doc/manuals/developers-reference/developers-reference.html
- Debian New Maintainers' Guide: https://www.debian.org/doc/manuals/maint-guide/
- WNPP: https://www.debian.org/devel/wnpp/
- Debian Mentors FAQ: https://wiki.debian.org/DebianMentorsFaq
- mentors.debian.net introduction for maintainers: https://mentors.debian.net/intro-maintainers/
