# libxml2

libxml2 is an XML toolkit implemented in C, originally developed for
the GNOME Project.

Official releases can be downloaded from
<https://download.gnome.org/sources/libxml2/>

The git repository is hosted on GNOME's GitLab server:
<https://gitlab.gnome.org/GNOME/libxml2>

Bugs should be reported at
<https://gitlab.gnome.org/GNOME/libxml2/-/issues>.

Documentation is available at
<https://gitlab.gnome.org/GNOME/libxml2/-/wikis>

## Fork Notes

This fork tracks upstream but adds e-reader focused hardening and process
artifacts:
- `NEWS.md` for fork-specific changes.
- `security_audit.md` for risk tracking and mitigations.
- `testresults.md` to record test runs and outcomes.

## License

This code is released under the MIT License, see the Copyright file.

## Security

This is open-source software written by hobbyists and maintained by
volunteers.

It's NOT recommended to use this software to process **untrusted data**.
There is a lot of ways that a malicious crafted xml could exploit a
hidden vulnerability in the software.

The software is provided "as is", without warranty of any kind,
express or implied. Use this software at your own risk.

To **report security bugs**, you can create a confidential issue with
the "security" label. We will review and work on it as a best effort.
But remember that this is a community project, maintained by volunteer
developers, so if you are concern about any important security bug
that's critical for you, feel free to collaborate and provide a patch.

The main rule is to be kind. Do not pressure developers to fix a CVE
or to work on a functionality that you need, because that won't work.
This is a community project, developers will work in the issues that
they consider interesting and when they want. All contributions are
welcome, so if something is important for you, you can always get
involved, implement it yourself and be part of the open source
community.

## DRM-Friendly Parsing (Optional)

If you need to enforce DRM-style controls over external resource loading,
use the resource loader and policy APIs:

```c
xmlParserCtxt *ctxt = xmlNewParserCtxt();
xmlCtxtSetResourceLoader(ctxt, my_loader, my_ctx);
xmlCtxtSetResourcePolicy(ctxt, my_policy, my_ctx);

/* Require a custom loader for any external resource. */
xmlCtxtSetOptions(ctxt, XML_PARSE_REQUIRE_LOADER |
                         XML_PARSE_NO_XXE |
                         XML_PARSE_NONET);
```

`XML_PARSE_REQUIRE_LOADER` will fail external loads unless a custom
resource loader is installed. The policy callback can approve or deny
loads before the loader is invoked.

## Error Reporting

For richer diagnostics, install a structured error handler and capture
`xmlError` details:

```c
static void
myStructuredError(void *ctx, xmlErrorPtr err) {
    (void)ctx;
    if (err == NULL)
        return;
    fprintf(stderr,
            "libxml2 error domain=%d code=%d level=%d file=%s line=%d col=%d\n"
            "message=%s\n"
            "str1=%s str2=%s str3=%s\n",
            err->domain, err->code, err->level,
            err->file ? err->file : "(none)",
            err->line, err->int2,
            err->message ? err->message : "(none)",
            err->str1 ? err->str1 : "(none)",
            err->str2 ? err->str2 : "(none)",
            err->str3 ? err->str3 : "(none)");
}

xmlSetStructuredErrorFunc(NULL, myStructuredError);
```

Resource loader/policy failures include extra context in `str1` (URI),
`str2` (resource type), and `str3` (reason). The message also includes
flags like `UNZIP` or `NETWORK`.

For general parser errors, `str3` is set to the current parse stage
when available (for example `prolog`, `content`, or `dtd`).

You can also enable a per-context error ring buffer to keep the last
N errors for postmortem analysis:

```c
xmlParserCtxt *ctxt = xmlNewParserCtxt();
xmlCtxtSetErrorRingSize(ctxt, 16);
/* ... parse ... */
int count = xmlCtxtGetErrorRing(ctxt, NULL, 0);
xmlError *errs = calloc(count, sizeof(*errs));
xmlCtxtGetErrorRing(ctxt, errs, count);
/* ... use errs ... */
for (int i = 0; i < count; i++) {
    xmlResetError(&errs[i]);
}
free(errs);
```

If you need structured telemetry, use `xmlErrorToJson`:

```c
xmlChar *json = NULL;
int len = 0;
if (xmlErrorToJson(err, &json, &len) == 0) {
    /* send json */
    xmlFree(json);
}
```

## Build instructions

libxml2 can be built with GNU Autotools, CMake or meson.

### Autotools (for POSIX systems like Linux, BSD, macOS)

If you build from a Git tree, you have to install Autotools and start
by generating the configuration files with:

    ./autogen.sh [configuration options]

If you build from a source tarball, extract the archive with:

    tar xf libxml2-xxx.tar.gz
    cd libxml2-xxx

Then you can configure and build the library:

    ./configure [configuration options]
    make

The following options disable or enable code modules and relevant symbols:

    --with-c14n             Canonical XML 1.0 support (on)
    --with-catalog          XML Catalogs support (on)
    --with-debug            debugging module (on)
    --with-docs             Build documentation (off)
    --with-history          history support for xmllint shell (off)
    --with-readline[=DIR]   use readline in DIR for shell (off)
    --with-html             HTML parser (on)
    --with-http             ABI compatibility for removed HTTP support (off)
    --with-iconv[=DIR]      iconv support (on)
    --with-icu              ICU support (off)
    --with-iso8859x         ISO-8859-X support if no iconv (on)
    --with-modules          dynamic modules support (on)
    --with-output           serialization support (on)
    --with-pattern          xmlPattern selection interface (on)
    --with-push             push parser interfaces (on)
    --with-python           Python bindings (off)
    --with-reader           xmlReader parsing interface (on)
    --with-regexps          regular expressions support (on)
    --with-relaxng          RELAX NG support (on)
    --with-sax1             older SAX1 interface (on)
    --with-schemas          XML Schemas 1.0 support (on)
    --with-schematron       Schematron support (off)
    --with-threads          multithreading support (on)
    --with-thread-alloc     per-thread malloc hooks (off)
    --with-valid            DTD validation support (on)
    --with-winpath          Windows path support (on for Windows)
    --with-writer           xmlWriter serialization interface (on)
    --with-xinclude         XInclude 1.0 support (on)
    --with-xpath            XPath 1.0 support (on)
    --with-xptr             XPointer support (on)
    --with-zlib[=DIR]       use libz in DIR (off)

Other options:

    --prefix=DIR            set installation prefix
    --with-minimum          build a minimally sized library (off)
    --with-legacy           maximum ABI compatibility (off)
    --with-secure-defaults  enable secure default parser options (off)

Secure defaults only affect legacy APIs without explicit options.
Callers that pass options (for example to `xmlReadMemory`) are unchanged.
When enabled, the legacy defaults add `XML_PARSE_NO_XXE`,
`XML_PARSE_NONET`, and `XML_PARSE_REQUIRE_LOADER`.

Note that by default, no optimization options are used. You have to
enable them manually, for example with:

    CFLAGS='-O2 -fno-semantic-interposition' ./configure

Now you can run the test suite with:

    make check

Please report test failures to the bug tracker.

Then you can install the library:

    make install

At that point you may have to rerun ldconfig or a similar utility to
update your list of installed shared libs.

### CMake (mainly for Windows)

Example commands:

    cmake -E tar xf libxml2-xxx.tar.xz
    cmake -S libxml2-xxx -B builddir [options]
    cmake --build builddir
    ctest --test-dir builddir
    cmake --install builddir

Common CMake options include:

    -D BUILD_SHARED_LIBS=OFF            # build static libraries
    -D CMAKE_BUILD_TYPE=Release         # specify build type (single-config)
    --config Release                    # specify build type (multi-config)
    -D CMAKE_INSTALL_PREFIX=/usr/local  # specify the install path
    -D LIBXML2_WITH_ICONV=OFF           # disable iconv
    -D LIBXML2_WITH_ZLIB=ON             # enable zlib
    -D LIBXML2_WITH_SECURE_DEFAULTS=ON  # enable secure default parser options

You can also open the libxml source directory with its CMakeLists.txt
directly in various IDEs such as CLion, QtCreator, or Visual Studio.

### Meson

Example commands:

    meson setup [options] builddir
    ninja -C builddir
    meson test -C builddir
    ninja -C builddir install

See the `meson_options.txt` file for options. For example:

    -Dprefix=$prefix
    -Dhistory=enabled
    -Dschemas=disabled
    -Dzlib=enabled
    -Dsecure-defaults=true

## Testing

The full test suite is run by your build system:

- Autotools: `make check`
- CMake: `ctest --test-dir builddir`
- Meson: `meson test -C builddir`

Additional targeted test executables are built in the tree. Typical runs:

- `./runtest`
- `./runxmlconf`
- `./runsuite`
- `./testlimits`
- `./testrecurse`

After any test run, update `testresults.md` with the command and result.

## Dependencies

libxml2 supports POSIX and Windows operating systems.

The iconv function is required for conversion of character encodings.
This function is part of POSIX.1-2001. If your platform doesn't provide
iconv, you need an external libiconv library, for example
[GNU libiconv](https://www.gnu.org/software/libiconv/). Using
[ICU](https://icu.unicode.org/) is also supported but discouraged.

The xmllint executable uses libreadline and libhistory if enabled.

### Build requirements

Besides build system tools, only a C compiler should be required.
Reconfiguration of the Autotools build requires the pkg.m4 macro from
pkg-config. Building the documentation requires Doxygen, xsltproc and the
DocBook 4 XSLT stylesheets. Building the Python bindings requires Doxygen.

## Contributing

The current version of the code can be found in GNOME's GitLab at
<https://gitlab.gnome.org/GNOME/libxml2>. The best way to get involved
is by creating issues and merge requests on GitLab.

All code must conform to C89 and pass the GitLab CI tests. Add regression
tests for new APIs or modules. Update `testresults.md` after running tests.

## Authors

- Daniel Veillard
- Bjorn Reese
- William Brack
- Igor Zlatkovic for the Windows port
- Aleksey Sanin
- Nick Wellnhofer
