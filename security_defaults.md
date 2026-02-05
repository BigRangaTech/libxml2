SPDX-License-Identifier: GPL-2.0-or-later

# Security Defaults Guide

This document describes recommended default settings for parsing untrusted
content in the e-reader stack. These are guidance values and should be
validated against your corpus of ebooks and malformed samples.

## Recommended Defaults
Apply these settings when parsing untrusted input:

- Use explicit parser options on every call.
- Set a strict amplification limit.
- Set a dictionary size limit.
- Avoid external entity expansion.

Suggested baseline values:
- `xmlSetMaxAmplificationDefault(10)`
- `xmlDictSetDefaultLimit(16 * 1024 * 1024)`

## Safe Defaults Build Flag
This fork provides an opt-in build flag to enable secure defaults for legacy
APIs that do not pass explicit options:

- Autotools: `--with-secure-defaults`
- CMake: `-D LIBXML2_WITH_SECURE_DEFAULTS=ON`
- Meson: `-Dsecure-defaults=true`

When enabled, legacy defaults add:
- `XML_PARSE_NO_XXE`
- `XML_PARSE_NONET`
- `XML_PARSE_REQUIRE_LOADER`

Callers that pass explicit options are unchanged.

## When To Use It
- Use the build flag in firmware or bundled builds where you control all
  call sites.
- Avoid enabling it for general-purpose system builds.

## Tuning Guidance
- Start with the baseline limits above.
- Increase limits only after measuring memory behavior on large books.
- Keep `XML_PARSE_NOENT` disabled for untrusted content.

## Tracking
Record any deviations from these defaults in `security_audit.md` with a
justification and the content types that require it.
