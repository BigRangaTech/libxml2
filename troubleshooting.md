SPDX-License-Identifier: GPL-2.0-or-later

# Troubleshooting

Common issues and fixes when parsing e-reader content with this fork.

## Resource Loader Errors
**Symptom**
- `I/O error : resource loader required ...` or `resource policy denied ...`

**Cause**
- `XML_PARSE_REQUIRE_LOADER` is enabled without a loader/policy.

**Fix**
- Install a resource loader and policy, or remove `XML_PARSE_REQUIRE_LOADER`
  for trusted inputs.

## External Entity / Network Access
**Symptom**
- External DTDs or entities fail to load.

**Cause**
- `XML_PARSE_NO_XXE` or `XML_PARSE_NONET` is enabled.

**Fix**
- Keep these options for untrusted content. Only disable them for trusted,
  fully local inputs.

## Entity Expansion Limits
**Symptom**
- `Maximum entity amplification factor exceeded`.

**Cause**
- Amplification limit was triggered by large or recursive entities.

**Fix**
- Review `xmlSetMaxAmplificationDefault` values. Increase only after testing
  with representative inputs.

## Dictionary Limit Reached
**Symptom**
- Parsing fails on very large documents.

**Cause**
- Dictionary size limit is too low.

**Fix**
- Tune `xmlDictSetDefaultLimit` and retest. Record changes in
  `security_audit.md`.

## Excessive Error Volume
**Symptom**
- Logs contain repeated or noisy errors.

**Fix**
- Enable `xmlCtxtSetErrorDedup` or `xmllint --error-dedup`.
- Use `--error-json-limit` or `--error-json-summary` for bounded output.

## Missing Structured Fields
**Symptom**
- `str1`, `str2`, or `str3` are empty.

**Cause**
- The specific error type may not populate optional context fields.

**Fix**
- Check `message`, `code`, `domain`, and `stage` fields for context.

## Unexpected Format or Encoding Errors
**Symptom**
- Errors about invalid characters or encoding.

**Cause**
- Input encoding metadata does not match the actual content.

**Fix**
- Normalize inputs before parsing or pass explicit encoding hints when
  available.
