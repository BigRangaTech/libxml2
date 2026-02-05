SPDX-License-Identifier: GPL-2.0-or-later

# E-Book Regression Test Set

This document defines a minimal, repeatable regression set for the e-reader
stack. It is intentionally small but representative.

## Goals
- Catch regressions in parsing common book structures.
- Include malformed/hostile inputs for safety checks.
- Keep runtime short enough for frequent use.

## Suggested Corpus (Keep Locally)
- 5–10 known-good books (EPUB, MOBI, AZW, KF8)
- 3 malformed XML samples (broken tags, invalid entities, bad encoding)
- 2 large documents for memory pressure
- 2 compressed inputs (zip with nested content)

## Test Runs
- libxml2: `make check-local`
- libxml2: `ctest --test-dir builddir`
- App/libmobi: smoke parse on each book in the corpus

## Recording
Record results in `testresults.md` with the reason (for example “ebook corpus
smoke”). If a test fails, log it in **Failed Tests** with the failure cause
and re-run after the fix.
