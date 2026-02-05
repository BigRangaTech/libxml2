SPDX-License-Identifier: GPL-2.0-or-later

# Security Audit Checklist

Use this checklist for periodic reviews and before releases. Record results
in `security_audit.md`.

## When to Run
- Before each release or external deployment.
- After any parser option, resource loader, or security limit change.
- After merging upstream changes that touch parsing or I/O.

## Checklist
- Parser options for untrusted input include:
  - `XML_PARSE_NO_XXE`
  - `XML_PARSE_NONET`
  - `XML_PARSE_REQUIRE_LOADER` when external loads are possible
- `XML_PARSE_NOENT` is not enabled for untrusted content.
- Amplification limit set (`xmlSetMaxAmplificationDefault`) and documented.
- Dictionary limit set (`xmlDictSetDefaultLimit`) and documented.
- External resource loader and policy are configured and tested.
- DTD validation disabled for untrusted content unless required and safe.
- XInclude disabled or tightly controlled for untrusted content.
- XPath/XPointer usage reviewed for unbounded operations.
- Catalog resolution does not allow network access.
- Compressed input handling has size limits and safe defaults.
- Error output redaction rules reviewed for PII.
- Structured error output formats validated (JSON/XML/CBOR/binary).
- Fuzzing targets run for DTD/HTML/schema paths (if available).
- Sanitizers run periodically (ASAN/UBSAN) for key parsers.
- CVE review performed and recorded with mitigations if needed.
- Regression tests updated for any new security-relevant API or switch.

## Evidence to Capture
- Test run logs (autotools, CMake; meson if used).
- Configuration flags used for release builds.
- Summary of any deviations from recommended defaults.

## Audit Log Template
- Date:
- Reviewer:
- Scope:
- Summary:
- Deviations:
- Tests run:
- Follow-ups:
