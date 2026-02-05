SPDX-License-Identifier: GPL-2.0-or-later

# E-Reader Integration Guide (libxml2 Fork)

This document outlines app-side integration guidance for Linux and Android
where libxml2 is used via libmobi. It focuses on safe defaults for untrusted
content, structured error reporting, and reproducible diagnostics.

## Goals
- Keep all libxml2 modules enabled and fully supported.
- Avoid breaking libmobi or app parsing behavior.
- Prevent external entity loading and network access for untrusted ebooks.
- Provide consistent error telemetry for triage and QA.

## libmobi-Ready Feature Checklist
These are libxml2 capabilities worth keeping ready so libmobi can adopt them
later without refactoring.

- Structured error output (JSON/XML) + ring buffer dumps for QA telemetry.
- Error deduplication to reduce noisy logs on malformed books.
- Redaction rules for PII in error outputs.
- Resource policy/loader hooks for DRM-friendly allowlists.
- Parser limits: amplification and dictionary size caps.
- Compact parsing flags for lower memory use.
- Optional xmlReader streaming for large NCX/XML parts.
- Canonicalization (C14N) support for hashing/integrity workflows.
- Optional schema/validation paths for QA builds only.
- Controlled catalog resolution for offline or curated identifiers.

## Parser Options
Use explicit parser options for all untrusted content. Prefer additive flags
and avoid changing libxml2 global defaults unless you fully control all call
sites.

### HTML (RawML)
Recommended flags for `htmlReadMemory` in libmobi:
- `HTML_PARSE_RECOVER`
- `HTML_PARSE_NONET`
- `HTML_PARSE_NOERROR`
- `HTML_PARSE_NOWARNING`
- `HTML_PARSE_COMPACT`

### XML (NCX)
Recommended flags for `xmlReadMemory` in libmobi:
- `XML_PARSE_RECOVER`
- `XML_PARSE_NONET`
- `XML_PARSE_NO_XXE`
- `XML_PARSE_NOERROR`
- `XML_PARSE_NOWARNING`
- `XML_PARSE_COMPACT`

Avoid `XML_PARSE_NOENT` for untrusted input.

## DRM-Friendly Resource Policy
If you need to allow only specific URI schemes or local paths, install a
resource policy and loader and require it at parse time:

```c
static int
myPolicy(void *user, const char *uri, xmlParserInputFlags flags) {
    (void)user;
    if (uri == NULL)
        return 0;
    if (!strncmp(uri, "appcache://", 11))
        return 1;
    if (!strncmp(uri, "file:///data/ereader/", 22))
        return 1;
    return 0;
}

xmlParserCtxt *ctxt = xmlNewParserCtxt();
xmlCtxtSetResourcePolicy(ctxt, myPolicy, NULL);
xmlCtxtSetResourceLoader(ctxt, myLoader, NULL);
xmlCtxtSetOptions(ctxt, XML_PARSE_REQUIRE_LOADER |
                         XML_PARSE_NO_XXE |
                         XML_PARSE_NONET);
```

`XML_PARSE_REQUIRE_LOADER` ensures that external resource loads go through
your policy and loader. Denied loads generate structured errors that include
URI and reason strings.

## Limits and Amplification
Set caps on amplification and dictionary size before parsing large or
malformed documents:

```c
xmlSetMaxAmplificationDefault(10);
xmlDictSetDefaultLimit(16 * 1024 * 1024);
```

Tune these values using a corpus of real books plus adversarial samples.
Record chosen values in app configuration so they can be audited and
changed without code changes.

## Error Reporting and Telemetry
Install a structured error handler and capture error rings for postmortem
analysis:

```c
xmlParserCtxt *ctxt = xmlNewParserCtxt();
xmlCtxtSetErrorRingSize(ctxt, 16);
xmlCtxtSetErrorDedup(ctxt, 1);
```

Use `xmlErrorToJson` or `xmlErrorToXml` to serialize errors for logging or
telemetry. If you need redaction, apply it in the app layer before writing
to disk or sending telemetry.

## Debugging With xmllint
For repro and QA, `xmllint` can emit structured error output:
- `--error-json` / `--error-xml`
- `--error-redact file,message,str1,str2,str3,window`
- `--error-ring` + `--error-ring-dump-bin-file` for compact dumps

## Testing
After changes, run:
- `make check-local`
- `ctest --test-dir builddir`

Record results in `testresults.md`.

## Android Notes
- Prefer explicit file access via app-managed paths rather than general
  `file://` URLs.
- If you allow `file://`, restrict to a fixed prefix (for example a
  dedicated book storage directory).
- Map libxml2 structured errors to Android logging categories for QA
  (`E`, `W`, `I`) and include the ebook ID and parse stage.
- Avoid loading resources from network schemes in production builds.

## Linux Notes
- Prefer explicit paths and avoid implicit network resolution.
- Use a fixed allowlist of URI schemes if external resources are needed.
