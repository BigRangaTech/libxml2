SPDX-License-Identifier: GPL-2.0-or-later

# Diagnostics Quickstart

This guide shows common `xmllint` commands for structured error output and
troubleshooting.

## JSON Output
Write one JSON object per error to stderr:

```sh
xmllint --error-json --noout book.xml
```

Write to a file and include a summary:

```sh
xmllint --error-json --error-json-file errors.json \
  --error-json-summary --noout book.xml
```

Pretty-printed array output:

```sh
xmllint --error-json --error-json-array --error-json-pretty --noout book.xml
```

## XML Output

```sh
xmllint --error-xml --noout book.xml
```

To write XML errors to a file:

```sh
xmllint --error-xml --error-xml-file errors.xml --noout book.xml
```

## Redaction
Redact fields that may contain PII or sensitive paths:

```sh
xmllint --error-json --error-redact file,str1,str2,str3 --noout book.xml
```

To redact everything, including JSON windows:

```sh
xmllint --error-json --error-redact all --noout book.xml
```

## Error Ring Buffer Dumps
Store the last N errors in a ring buffer and dump them:

```sh
xmllint --error-ring 32 --error-ring-dump --noout book.xml
```

Write CBOR or binary ring dumps to files:

```sh
xmllint --error-ring 32 --error-ring-dump-cbor-file errors.cbor --noout book.xml
xmllint --error-ring 32 --error-ring-dump-bin-file errors.bin --noout book.xml
```

The binary ring dump uses a simple header and length-prefixed JSON frames.
See `error_output_formats.md` for details.

## Syslog Output
Route errors to syslog (with a selectable facility):

```sh
xmllint --error-syslog --error-syslog-facility user --noout book.xml
```
