SPDX-License-Identifier: GPL-2.0-or-later

# Error Output Formats (xmllint)

This document describes the structured error output formats added to xmllint.
It is intended for tooling, QA, and log ingestion pipelines.

## Overview
xmllint can emit structured errors in JSON, XML, CBOR, or a compact binary
ring dump. All formats represent the same core data fields and can be
redacted with `--error-redact`.

Common switches:
- `--error-json` or `--error-json-file FILE`
- `--error-xml` or `--error-xml-file FILE`
- `--error-ring N` plus `--error-ring-dump` or file options
- `--error-ring-dump-cbor-file FILE`
- `--error-ring-dump-bin-file FILE`
- `--error-redact LIST`

## Core Fields
Each structured error represents a libxml2 `xmlError` with additional
context. The fields may be absent if unknown.

- `domain`: numeric error domain
- `code`: numeric error code
- `level`: error level
- `file`: source filename or URI
- `line`: line number
- `int2`: column or extra integer context
- `message`: human-readable message
- `str1`, `str2`, `str3`: additional context strings
- `timestamp`: epoch time in seconds (xmllint only)
- `stage`: parse stage tag (for example `prolog`, `content`, `dtd`)
- `resource_type`: loader policy type string
- `resource_flags`: loader policy flags string

## JSON Output
`--error-json` writes a JSON object per error to stderr. Options change the
layout:

- `--error-json-array` wraps errors in a JSON array
- `--error-json-pretty` pretty prints output
- `--error-json-limit N` stops after N errors
- `--error-json-summary` appends a summary object
- `--error-json-window N` adds a source window
- `--error-json-checksum` adds checksums

Example (single error object):

```json
{"domain":1,"code":76,"level":2,"file":"book.xml","line":42,"int2":17,
"message":"Entity 'foo' not defined","str1":"foo","str2":null,"str3":"content",
"timestamp":1738710000}
```

If `--error-json-summary` is enabled, the final object includes counts and
per-stage timing values.

## XML Output
`--error-xml` writes one `<error>` element per error, wrapped in a single
root element when array mode is enabled internally. Fields are represented
as child elements.

Example:

```xml
<error>
  <domain>1</domain>
  <code>76</code>
  <level>2</level>
  <file>book.xml</file>
  <line>42</line>
  <int2>17</int2>
  <message>Entity 'foo' not defined</message>
  <str1>foo</str1>
  <str3>content</str3>
</error>
```

## CBOR Output
`--error-ring-dump-cbor-file FILE` writes the ring buffer in CBOR. Each
entry is a CBOR map with the same keys as the JSON format. This is intended
for size-efficient transport.

## Binary Ring Dump
`--error-ring-dump-bin-file FILE` writes a compact binary stream:

- 4 bytes ASCII magic: `XERB`
- 4 bytes little-endian version (currently 1)
- Repeating frames:
  - 4 bytes little-endian length
  - JSON bytes for one error object

This format is easy to parse in low-level tooling while remaining
self-describing via JSON frames.

## Redaction
`--error-redact LIST` replaces selected fields with `[redacted]`. Supported
tokens:

- `file`, `message`, `str1`, `str2`, `str3`
- `window` to remove JSON source windows
- `all` and `none`

Redaction applies to JSON, XML, CBOR, and binary ring dumps. JSON source
windows are set to `null` when redacted.
