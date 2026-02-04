# Test Results

Record test runs here. Update after every test run, even if failures occur.

## Latest
- 2026-02-04: Autotools + CMake tests passed after error ring + JSON helper.

## History
| Date | Build System | Config | Command | Result | Notes |
| --- | --- | --- | --- | --- | --- |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | PASS | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | PASS* | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | PASS | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | PASS* | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | PASS | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | PASS* | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake --build builddir && ctest --test-dir builddir` | PASS | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | PASS* | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `ctest --test-dir builddir` | PASS | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | PASS* | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-04 | CMake | default | `cmake -S . -B builddir && cmake --build builddir && ctest --test-dir builddir` | PASS | 23/23 tests passed |
| 2026-02-04 | Autotools | default | `make check` | PASS* | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-03 | CMake | Release | `cmake -S . -B builddir -D CMAKE_BUILD_TYPE=Release && cmake --build builddir && ctest --test-dir builddir` | PASS | 22/22 tests passed |
| 2026-02-03 | Autotools | default | `./autogen.sh && make -j$(nproc) && make check` | PASS* | External suites missing: XML Conformance (xmlconf) and xstc metadata files |
| 2026-02-03 | Autotools | default | `make check` | PASS* | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
| 2026-02-03 | CMake | Release | `cmake --build builddir && ctest --test-dir builddir` | PASS | 22/22 tests passed |
| 2026-02-03 | Autotools | default | `make check` | PASS* | XML Conformance ran (5 expected errors). XSTC ran (Sun/MS suites report expected errors). |
