SPDX-License-Identifier: GPL-2.0-or-later

# Release Process (Fork)

This document defines the release steps for the e-reader libxml2 fork.

## Pre-Release Checklist
- Review `security_audit_checklist.md` and update `security_audit.md`.
- Update `NEWS.md` with a dated summary.
- Ensure new APIs or switches have tests.
- Run tests:
  - `make check-local`
  - `ctest --test-dir builddir`
  - `meson test -C builddir` if Meson is part of your release workflow
- Record results in `testresults.md`.
- Verify documentation:
  - `README.md`
  - `ereader_integration.md`
  - `security_defaults.md`
  - `error_output_formats.md`

## Versioning
- If you tag releases, create a tag consistent with your internal scheme.
- If you do not tag releases, record the git commit hash in `NEWS.md`.

## Release Artifacts
- Source archive or repo snapshot.
- Build artifacts for Linux and Android targets.
- Test logs (or references to CI runs).

## Post-Release
- Record the release date and identifier in `NEWS.md`.
- Note any deviations from the security defaults in `security_audit.md`.
