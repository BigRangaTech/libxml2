# Security Audit Notes

Purpose: track potential risk areas, audit status, and mitigations for this fork.
This is a living document; update it whenever security-relevant behavior changes.

## Audit Status
- Last reviewed: 2026-02-04
- Scope: parser defaults, entity expansion limits, dictionary limits, resource loading

## Potential Risk Areas (Non-Exhaustive)
- External entity loading (XXE) via DTDs and parameter entities.
- Entity expansion amplification (billion laughs / exponential growth).
- DTD validation and schema validation (algorithmic complexity, memory growth).
- XInclude processing (external resource fetch, path resolution).
- XPath evaluation (potential unbounded operations).
- Compressed input handling (unzip bombs, size limits).
- Catalog resolution and network access.
- Large text nodes, deep recursion, and large dictionaries.

## Mitigations / Controls
- Use `XML_PARSE_NO_XXE` and `XML_PARSE_NONET` for untrusted input.
- Set a max amplification factor via `xmlSetMaxAmplificationDefault`.
- Enforce dictionary limits via `xmlDictSetDefaultLimit`.
- Use `XML_PARSE_REQUIRE_LOADER` with `xmlCtxtSetResourceLoader` to enforce
  DRM-style resource control.
- Use `xmlCtxtSetResourcePolicy` to approve/deny external resource loads.
- Optionally build with secure defaults (`--with-secure-defaults`) for
  legacy APIs that don't take explicit options.
- Avoid DTD validation and external resource loading for untrusted data.

## Open Questions
- Which libmobi parsing paths still need additional hardening?

## Change Log
- 2026-02-04: Added opt-in build flag for secure default parser options.
- 2026-02-03: Added global defaults for max amplification and dict limits.
- 2026-02-03: Added xmllint `--no-xxe` option and RNG `/dev/urandom` fallback.
- 2026-02-03: Added resource loader requirement and resource policy hooks.
