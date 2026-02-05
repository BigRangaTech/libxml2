SPDX-License-Identifier: GPL-2.0-or-later

# Test Results

Record test runs here. Update after every test run, even if failures occur.
When a test fails, log it in **Failed Tests** with the reason and failure
cause, record it in **History** as a failed test, and then re-test after
fixes. Use the Failed Tests section for full details.

## Latest
- 2026-02-05: Autotools check-local + CMake ctest passed after NEWS process note. Reason: documentation/process updates. Duration: check-local n/a; ctest 8.27s.

## History
| Date | Build System | Config | Command | Reason | Duration | Result | Failure Reason | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-02-05 | Autotools | default | `make check-local` | NEWS update (process note) | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | NEWS update (process note) | 8.27s | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | SPDX headers + failure tracking fields | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | SPDX headers + failure tracking fields | 8.29s | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | SPDX headers + testresults format | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | SPDX headers + testresults format | 8.39s | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-05 | Autotools | default | `make check-local` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-05 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake -S . -B builddir && cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-03 | CMake | Release | `cmake -S . -B builddir -D CMAKE_BUILD_TYPE=Release && cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 22/22 tests passed |
| 2026-02-03 | Autotools | default | `./autogen.sh && make -j$(nproc) && make check` | n/a | n/a | PASS* | n/a | External suites missing: XML Conformance (xmlconf) and xstc metadata files |
| 2026-02-03 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-03 | CMake | Release | `cmake --build builddir && ctest --test-dir builddir` | n/a | n/a | PASS | n/a | 22/22 tests passed |
| 2026-02-03 | Autotools | default | `make check` | n/a | n/a | PASS* | n/a | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |

## Failed Tests
Record failures here (even if later resolved). Include the reason for the run, duration if known, and the failure cause.
If you saw failures that weren’t recorded, add best‑known details here.

- Template: `YYYY-MM-DD` | Command | Reason | Duration | Failure Reason | Notes
