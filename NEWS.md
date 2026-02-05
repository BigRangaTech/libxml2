SPDX-License-Identifier: GPL-2.0-or-later

# News (Fork Changelog)

This file tracks notable changes in this fork. Keep entries short and focused
on what changed and why it matters.

## 2026-02-04
- Security/Hardening: Added an opt-in build flag to enable secure default parser
  options for legacy APIs (`--with-secure-defaults`,
  `-D LIBXML2_WITH_SECURE_DEFAULTS=ON`, `-Dsecure-defaults=true`).
- Diagnostics: Improved resource load error reporting with resource type,
  flags, and reason for loader/policy failures.
- Diagnostics: Added parse-stage tagging in structured errors and a one-time
  warning when error/warning limits are reached.
- Diagnostics: Added per-context error ring buffers and a JSON formatter
  helper for structured telemetry.
- Diagnostics: Added xmllint switches for error rings and JSON error output.
- Diagnostics: Extended xmllint JSON output with filenames, options, stages,
  resource types, timestamps, file targets, array mode, pretty printing,
  and output limits.
- Diagnostics: Added JSON summaries, input windows, checksums, warning
  channels, syslog output, and CBOR ring dumps in xmllint.
## 2026-02-05
- Docs: Added `ereader_integration.md` with app-side integration guidance for
  Linux and Android, including parser options, limits, and error telemetry.
- Docs: Added `error_output_formats.md` for structured error output formats.
- Docs: Added `security_defaults.md` for recommended parsing defaults.
- Docs: Added `security_audit_checklist.md` and `release_process.md`.
- Docs: Added `diagnostics_quickstart.md` and `troubleshooting.md`.
- Licensing: Marked fork as standalone and documented GPL-2.0-or-later
  distribution terms (MIT notices retained).
- Process: Added SPDX headers to core documentation and expanded
  `testresults.md` with reason/duration/failure tracking.
- Diagnostics: Added per-context error deduplication APIs to suppress repeated
  errors and a corresponding `xmllint --error-dedup` switch.
- Diagnostics: Extended JSON summaries with per-stage counts and timing data.
- Diagnostics: Added `xmlErrorToXml` for structured XML error output.
- Diagnostics: Added `xmllint --error-xml`/`--error-xml-file` and redaction
  controls via `--error-redact`.
- Diagnostics: Added binary error ring dumps as length-prefixed JSON frames
  (`--error-ring-dump-bin-file`).
## 2026-02-03
- Security: Added `/dev/urandom` fallback for RNG seeding on older POSIX
  systems to avoid weak time-based seeds when no modern entropy API exists.
- Security: Added `--no-xxe` to `xmllint` to disable external entity loading.
- Security/Hardening: Added global defaults for max amplification and dictionary
  size limits (`xmlSetMaxAmplificationDefault`, `xmlDictSetDefaultLimit`) and
  apply them to new parser contexts and dictionaries.
- Security/DRM: Added `XML_PARSE_REQUIRE_LOADER` plus resource policy hooks
  (`xmlCtxtSetResourcePolicy`, `xmlSetResourcePolicy`) to enforce external
  resource control.
- Process: Added `ROADMAP.md`, `security_audit.md`, and `testresults.md` to keep
  planning, security review, and test status up to date.
