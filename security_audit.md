# Security Audit Notes

Purpose: track potential risk areas, audit status, and mitigations for this fork.
This is a living document; update it whenever security-relevant behavior changes.

## Audit Status
- Last reviewed: 2026-02-03
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
- Avoid DTD validation and external resource loading for untrusted data.

## Open Questions
- Do we want library-wide safe defaults for legacy APIs (opt-in build flag)?
- Which libmobi parsing paths still need additional hardening?

## Change Log
- 2026-02-03: Added global defaults for max amplification and dict limits.
- 2026-02-03: Added xmllint `--no-xxe` option and RNG `/dev/urandom` fallback.
