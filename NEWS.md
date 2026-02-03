# News (Fork Changelog)

This file tracks notable changes in this fork. Keep entries short and focused
on what changed and why it matters.

## 2026-02-03
- Security: Added `/dev/urandom` fallback for RNG seeding on older POSIX
  systems to avoid weak time-based seeds when no modern entropy API exists.
- Security: Added `--no-xxe` to `xmllint` to disable external entity loading.
- Security/Hardening: Added global defaults for max amplification and dictionary
  size limits (`xmlSetMaxAmplificationDefault`, `xmlDictSetDefaultLimit`) and
  apply them to new parser contexts and dictionaries.
- Process: Added `ROADMAP.md`, `security_audit.md`, and `testresults.md` to keep
  planning, security review, and test status up to date.
