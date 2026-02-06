SPDX-License-Identifier: GPL-2.0-or-later

# libxml2 + libmobi Integration Roadmap (Draft v0.2)

Updated: 2026-02-06

Goal: prioritize security, then performance, then features, then API/switches, then build size, then compatibility for the e-reader stack (Linux + Android) where libxml2 is used via libmobi.

Scope: libxml2 fork changes and app-side integration changes in the e-reader app and libmobi that affect security/perf. Avoid breaking libmobi unless explicitly accepted.

Constraints:
- Monocypher stays app-side (no new libxml2 dependency).
- libxml2 is a dependency of libmobi and the app; prefer additive changes.
- Keep all modules fully supported; focus on stability and security, not feature trimming.
- Fork is maintained as standalone; upstream merges are optional.
- Distribution license: GPL-2.0-or-later (upstream MIT notices retained).

Current State (2026-02-06):
- Fork matches upstream master at commit `2cc58340`.
- Added `/dev/urandom` fallback for RNG seeding on older POSIX systems in `dict.c`.
- Added xmllint `--no-xxe` switch and documentation in `xmllint.c` and `doc/xmllint.xml`.
- libmobi integration notes are maintained in the libmobi fork (`AGENTS.md`, `WHATS_NEW.md`).

## Progress (2026-02-06)
Completed (Documentation reflects implemented work in this fork):
- Added `xmllint --no-xxe`.
- Added secure-defaults build flag (opt-in safe defaults for legacy APIs).
- Added resource loader/policy enforcement and richer error context.
- Added per-context error ring buffers and error deduplication.
- Added structured JSON and XML error output helpers.
- Added xmllint structured error output switches, redaction, and ring dumps
  (JSON, XML, CBOR, binary frames).
- Added global defaults for max amplification and dictionary limits.
- Added `ereader_integration.md` for app-side guidance.
- Libmobi integration:
  - Build: `./configure --with-libxml2=libxml2` uses bundled fork and `make`
    builds `libxml2/` first; CMake `add_subdirectory(libxml2)` when present.
  - Parsing flags in libmobi: HTML uses `HTML_PARSE_NONET | HTML_PARSE_COMPACT`;
    NCX uses `XML_PARSE_NO_XXE | XML_PARSE_COMPACT`.
- Test signal (libmobi): `make test` PASS 11, XFAIL 1 (sample-invalid-indx.fail).

Documentation Status:
- `README.md` covers secure-defaults build options and testing commands.
- `ereader_integration.md` covers recommended parser flags and limits.
- `doc/xmllint.xml` documents structured error output switches.
- `error_output_formats.md` documents JSON, XML, CBOR, and binary ring formats.
- `security_defaults.md` documents recommended secure defaults and tuning.
- `security_audit_checklist.md` defines periodic review steps.
- `release_process.md` defines release steps for this fork.
- `diagnostics_quickstart.md` provides structured error usage examples.
- `troubleshooting.md` documents common parse failures and fixes.

## Libmobi Integration Contract
- Build discovery: libmobi prefers bundled `libxml2/` when present; external
  libxml2 can be used with `--with-libxml2=/path`.
- Parser flags: `HTML_PARSE_NONET | HTML_PARSE_COMPACT` for HTML, and
  `XML_PARSE_NO_XXE | XML_PARSE_COMPACT` for NCX.
- Size guards: libmobi must check `size_t` to `int` casts before calling
  `htmlReadMemory`/`xmlReadMemory`; skip/scan fallback for oversized inputs.
- No DTD expansion for untrusted input (avoid `XML_PARSE_NOENT`).
- Error handling: prefer structured error capture (ring buffer) where available;
  map to libmobi JSON diagnostics when exposed.

## Integration Tasks (libmobi)
- Wire libxml2 structured error output into libmobi diagnostics (JSON path).
- Ensure secure-defaults build flag is documented in libmobi build guide.
- Add CI job that builds libmobi with bundled libxml2 and runs the test corpus.

## Phase 0: Baseline & Inventory
- Confirm libmobi usage paths (which libxml2 APIs, options, and parsing modes it uses).
- Inventory app-side parsing entry points (where the app calls libmobi/libxml2).
- Capture current build matrix for Linux + Android (NDK versions, libc, CPU arch).
- Document current parser options used in production (XML_PARSE_* and HTML_PARSE_*).
- Establish a minimal regression test set (smoke parse, sample ebooks, malformed files).
- Record a libmobi-ready feature checklist (structured errors, limits, DRM policy,
  compact parsing, xmlReader, C14N, optional validation, catalogs).

### Baseline Notes (Fill In)
- Linux build matrix: distro, compiler, libc, CPU arch
- Android build matrix: NDK version, API levels, ABI list
- Parser flags in production:
  - HTML: (record here)
  - XML: (record here)
- Regression corpus location: (record local path or repo)

## Phase 1: Security Hardening (Highest Priority)
- Enforce safe defaults in app integration: `XML_PARSE_NO_XXE | XML_PARSE_NONET`, avoid `XML_PARSE_NOENT` for untrusted content.
- App-side: add `XML_PARSE_NO_XXE` to libmobi `xmlReadMemory` calls in `src/parse_rawml.c` (NCX parsing). (done in libmobi fork)
- App-side: confirm HTML parsing paths use `HTML_PARSE_NONET` (already set) and avoid any entity expansion. (libmobi also adds `HTML_PARSE_COMPACT`)
- Add DRM-friendly controls: `XML_PARSE_REQUIRE_LOADER` and resource policy hooks.
- App-side: define an allowlist of URL schemes/paths for resource loading
  (for example `appcache://` and `file:///data/ereader/`).
- App-side: install a structured error handler to capture libxml2 error
  context (URI/type/reason) and map it to ebook IDs and parse stages.
- Library-side: ensure error throttling emits a single "limit reached"
  message and includes parse stage info where available.
- Library-side: add a per-context error ring buffer and JSON formatter
  helper for app telemetry.
- Library-side: add error fingerprinting, schema versioning, and per-file
  error summaries for stable telemetry and triage.
- Library-side: add error deduplication after N repeats to reduce noise.
- App-side: define a severity mapping table from libxml2 error levels to
  user-facing severity.
- Library-side: add optional structured XML error output (for systems that
  can't consume JSON).
- Library-side: add per-stage timing and error counts in summaries for
  lightweight profiling.
- Library-side: add optional binary export (protobuf/flatbuffer) for error
  ring dumps when JSON/CBOR is too large.
- App-side: add configurable error redaction rules for PII (paths/URIs).
- Add a build-time switch to hard-enforce safe defaults in libxml2 for legacy APIs (opt-in; does not change default behavior unless enabled).
- Set a strict amplification factor via `xmlCtxtSetMaxAmplification` for untrusted input.
- Add/enable limits for dictionary size and node sizes when reading untrusted documents.
- Review recent CVEs to verify coverage in this fork and identify any downstream mitigations needed.
- Add guidance on disabling DTD validation for untrusted content.

## Phase 2: Performance & Memory
- Prefer streaming APIs (`xmlReader`) where libmobi permits it.
- Ensure `XML_PARSE_COMPACT` is enabled for memory reduction.
- Evaluate memory caps per parse (max document size, max dictionary size).
- Add profiling notes for typical e-reader content and large/edge-case documents.

## Phase 3: Features (App-Side Monocypher)
- App-side: use Monocypher to hash canonicalized XML content (C14N) for integrity checks.
- App-side: choose hash algorithm (BLAKE3 for speed or SHA-256 for compatibility).
- App-side: implement streaming hash adapter (init/update/final) for large documents.
- App-side: optional signature verification for packaged content if needed (future).
- Library-side: ensure canonicalization paths are stable and documented for app use.
- C++ API: provide a thin C++ wrapper over Monocypher C functions (RAII for ctx).
- Rust bindings: create a small `monocypher-sys` + safe wrapper crate (bindgen or handwritten).
- Python bindings: expose a minimal hashing API via `cffi` or `pybind11`.

## Phase 4: API/Switches
- Expose a CLI or build‑time switch for `--no-xxe` in xmllint (already added in this fork).
- Add a libxml2 config toggle to enable safe defaults globally when built for e-reader firmware.
- Add a switch to set max amplification from environment or config.
- Add xmllint switches for richer error output and routing:
  `--error-json-array`, `--error-json-pretty`, `--error-json-limit`,
  `--error-json-summary`, `--error-json-window`, `--error-json-checksum`,
  `--error-json-warn-file`, `--error-ring`, `--error-ring-dump`,
  `--error-ring-dump-file`, `--error-ring-dump-cbor-file`,
  `--error-syslog`, `--error-syslog-facility`,
  `--error-xml`, `--error-xml-file`, `--error-redact`,
  `--error-ring-dump-bin-file`, `--error-dedup`.

## Phase 5: Build Size & Compatibility
- Keep all modules enabled; do not trim features for this fork.
- Focus on compatibility across Linux + Android and stable behavior under stress.
- Verify ABI compatibility with existing app builds.
- Maintain Android NDK compatibility (avoid APIs missing on older API levels).

## Decisions Needed (Tracked)
- D1: Enforce safe defaults at library level or app level only. Owner: TBD. Target: TBD.
- D2: Confirm which XML features libmobi strictly requires (DTD, XInclude, XPath, Schemas, HTML). Owner: TBD. Target: TBD.
- D3: Define acceptable performance vs. security tradeoffs for malformed files. Owner: TBD. Target: TBD.
- D4: Primary hash algorithm: BLAKE3. Output format: raw bytes with optional hex helper. Owner: Jessie. Target: TBD.
- D5: Bindings strategy: Rust `bindgen` + safe wrapper; Python `cffi`. Owner: Jessie. Target: TBD.

## Success Criteria
- No XXE or external entity loading in production for untrusted input.
- No regression in libmobi parsing of known‑good content.
- Measurable memory stability on large/hostile inputs.

## Backlog (Unordered)
- Add fuzz harnesses for high‑risk parsers (DTD, schema, HTML).
- Add defensive limits for XPath (opLimit) in any evaluation paths used by libmobi.
- Improve error reporting hooks for better crash triage in production.
