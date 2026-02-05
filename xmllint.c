/*
 * xmllint.c : a small tester program for XML input.
 *
 * See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */

#include "libxml.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#ifndef _WIN32
#include <syslog.h>
#endif

#ifdef _WIN32
  #include <io.h>
  #include <sys/timeb.h>
#else
  #include <sys/time.h>
  #include <unistd.h>
#endif

#if HAVE_DECL_MMAP
  #include <sys/mman.h>
  #include <sys/stat.h>
  /* seems needed for Solaris */
  #ifndef MAP_FAILED
    #define MAP_FAILED ((void *) -1)
  #endif
#endif

#ifdef LIBXML_ZLIB_ENABLED
  #include <zlib.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/debugXML.h>
#include <libxml/xmlerror.h>
#ifdef LIBXML_XINCLUDE_ENABLED
#include <libxml/xinclude.h>
#endif
#ifdef LIBXML_CATALOG_ENABLED
#include <libxml/catalog.h>
#endif
#include <libxml/xmlreader.h>
#ifdef LIBXML_SCHEMATRON_ENABLED
#include <libxml/schematron.h>
#endif
#ifdef LIBXML_RELAXNG_ENABLED
#include <libxml/relaxng.h>
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
#include <libxml/xmlschemas.h>
#endif
#ifdef LIBXML_PATTERN_ENABLED
#include <libxml/pattern.h>
#endif
#ifdef LIBXML_C14N_ENABLED
#include <libxml/c14n.h>
#endif
#ifdef LIBXML_OUTPUT_ENABLED
#include <libxml/xmlsave.h>
#endif

#include "private/lint.h"

#define XMLLINT_JSON_SCHEMA_VERSION 1

#ifndef STDIN_FILENO
  #define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
  #define STDOUT_FILENO 1
#endif

#define MAX_PATHS 64

#ifdef _WIN32
  #define PATH_SEPARATOR ';'
#else
  #define PATH_SEPARATOR ':'
#endif

typedef enum {
    XMLLINT_RETURN_OK = 0,	    /* No error */
    XMLLINT_ERR_UNCLASS = 1,	    /* Unclassified */
    XMLLINT_ERR_DTD = 2,	    /* Error in DTD */
    XMLLINT_ERR_VALID = 3,	    /* Validation error */
    XMLLINT_ERR_RDFILE = 4,	    /* Wellformedness or IO error */
    XMLLINT_ERR_SCHEMACOMP = 5,	    /* Schema compilation */
    XMLLINT_ERR_OUT = 6,	    /* Error writing output */
    XMLLINT_ERR_SCHEMAPAT = 7,	    /* Error in schema pattern */
    /*XMLLINT_ERR_RDREGIS = 8,*/
    XMLLINT_ERR_MEM = 9,	    /* Out of memory error */
    XMLLINT_ERR_XPATH = 10,	    /* XPath evaluation error */
    XMLLINT_ERR_XPATH_EMPTY = 11    /* XPath result is empty */
} xmllintReturnCode;

#ifdef _WIN32
typedef __time64_t xmlSeconds;
#else
typedef time_t xmlSeconds;
#endif

typedef struct {
   xmlSeconds sec;
   int usec;
} xmlTime;

/* Boolean xmllint application options */
typedef enum {
    /** Do not build a tree but work just at the SAX level */
    XML_LINT_SAX_ENABLED = (1 << 0),
    /** run a navigating shell */
    XML_LINT_NAVIGATING_SHELL = (1 << 1),
    /** Show additional debug information */
    XML_LINT_DEBUG_ENABLED = (1 << 2),
    /** Test the internal copy implementation */
    XML_LINT_COPY_ENABLED = (1 << 3),
    /** Turn on gzip compression of output */
    XML_LINT_ZLIB_COMPRESSION = (1 << 4),
    /** Save in W3C canonical format v1.0 (with comments) */
    XML_LINT_CANONICAL_V1_0 = (1 << 5),
    /** Save in W3C canonical format v1.1 (with comments) */
    XML_LINT_CANONICAL_V1_1 = (1 << 6),
    /** Save exclusive canonical format (with comments) */
    XML_LINT_CANONICAL_EXE = (1 << 7),
    /** Do a posteriori validation, i.e after parsing */
    XML_LINT_POST_VALIDATION = (1 << 8),
    /** Ad-hoc test for valid insertions */
    XML_LINT_VALID_INSERTIONS = (1 << 9),
    /** Use the HTML parser */
    XML_LINT_HTML_ENABLED = (1 << 10),
    /** Force to use the XML serializer when using XML_LINT_HTML_ENABLED */
    XML_LINT_XML_OUT = (1 << 11),
    /** Use the push mode of the parser */
    XML_LINT_PUSH_ENABLED = (1 << 12),
    /** Parse from memory */
    XML_LINT_MEMORY = (1 << 13),
    /** Do XInclude processing */
    XML_LINT_XINCLUDE = (1 << 14),
    /** Be quiet when succeeded */
    XML_LINT_QUIET = (1 << 15),
    /** Print some timings */
    XML_LINT_TIMINGS = (1 << 16),
    /** Generate a small doc on the fly */
    XML_LINT_GENERATE = (1 << 17),
    /** Remove the DOCTYPE of the input docs */
    XML_LINT_DROP_DTD = (1 << 18),
    /** Use the streaming interface to process very large files */
    XML_LINT_USE_STREAMING = (1 << 19),
    /** Create a reader and walk though the resulting doc */
    XML_LINT_USE_WALKER = (1 << 20),
    /** use SGML catalogs from $SGML_CATALOG_FILES */
    XML_LINT_USE_CATALOGS = (1 << 21),
    /** Deactivate all catalogs */
    XML_LINT_USE_NO_CATALOGS = (1 << 22),
    /** Print trace of all external entities loaded */
    XML_LINT_USE_LOAD_TRACE = (1 << 23),
    /** Return application failure if document has any namespace errors */
    XML_LINT_STRICT_NAMESPACE = (1 << 24)


} xmllintAppOptions;

typedef struct {
    int flag;
    const char *name;
} xmllintOptName;

typedef struct {
    int code;
    unsigned count;
} xmllintCodeCount;

typedef enum {
    XMLLINT_REDACT_FILE = (1u << 0),
    XMLLINT_REDACT_MESSAGE = (1u << 1),
    XMLLINT_REDACT_STR1 = (1u << 2),
    XMLLINT_REDACT_STR2 = (1u << 3),
    XMLLINT_REDACT_STR3 = (1u << 4),
    XMLLINT_REDACT_WINDOW = (1u << 5)
} xmllintRedactFlags;

#define XMLLINT_REDACT_ALL (XMLLINT_REDACT_FILE | \
                            XMLLINT_REDACT_MESSAGE | \
                            XMLLINT_REDACT_STR1 | \
                            XMLLINT_REDACT_STR2 | \
                            XMLLINT_REDACT_STR3 | \
                            XMLLINT_REDACT_WINDOW)

typedef struct {
    char *name;
    unsigned count;
    long long firstMs;
    long long lastMs;
} xmllintStageCount;

typedef struct {
    FILE *errStream;
    xmlParserCtxtPtr ctxt;
    xmlResourceLoader defaultResourceLoader;

    int version;
    int maxmem;
    int callbacks;
    int noout;
#ifdef LIBXML_OUTPUT_ENABLED
    const char *output;
    const char *encoding;
    const char *indentString;
    int format;
#endif /* LIBXML_OUTPUT_ENABLED */
#ifdef LIBXML_VALID_ENABLED
    const char *dtdvalid;
    const char *dtdvalidfpi;
#endif
#ifdef LIBXML_RELAXNG_ENABLED
    const char *relaxng;
    xmlRelaxNGPtr relaxngschemas;
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
    const char *schema;
    xmlSchemaPtr wxschemas;
#endif
#ifdef LIBXML_SCHEMATRON_ENABLED
    const char *schematron;
    xmlSchematronPtr wxschematron;
#endif
    int repeat;
#ifdef LIBXML_HTML_ENABLED
    int htmlOptions;
#endif
#if HAVE_DECL_MMAP
    char *memoryData;
    size_t memorySize;
#endif
    xmllintReturnCode progresult;
#ifdef LIBXML_READER_ENABLED
#ifdef LIBXML_PATTERN_ENABLED
    const char *pattern;
    xmlPatternPtr patternc;
    xmlStreamCtxtPtr patstream;
#endif
#endif /* LIBXML_READER_ENABLED */
#ifdef LIBXML_XPATH_ENABLED
    const char *xpathquery;
    const char *xpathsep;
#endif
    int parseOptions;
    unsigned appOptions;
    unsigned maxAmpl;
    int errorRingSize;
    int errorDedupLimit;
    int errorRingDump;
    int errorXml;
    int errorJson;
    int errorJsonArray;
    int errorJsonPretty;
    int errorJsonLimit;
    int errorJsonCount;
    int errorJsonArrayOpen;
    int errorJsonArrayCount;
    const char *errorJsonArrayFile;
    int errorJsonSummary;
    int errorJsonWindow;
    int errorJsonChecksum;
    int errorSyslog;
    int errorSyslogFacility;
    unsigned errorRedactFlags;
    const char *errorRingDumpFile;
    const char *errorRingDumpCborFile;
    const char *errorRingDumpBinFile;
    const char *errorXmlFile;
    const char *errorJsonFile;
    const char *errorJsonWarnFile;
    FILE *errorRingDumpStream;
    FILE *errorRingDumpCborStream;
    FILE *errorRingDumpBinStream;
    FILE *errorXmlStream;
    FILE *errorJsonStream;
    FILE *errorJsonWarnStream;
    unsigned long long errorChecksum;
    int checksumValid;
    unsigned errorDomainCounts[XML_FROM_URI + 1];
    unsigned errorLevelCounts[4];
    xmllintCodeCount *errorCodeCounts;
    int errorCodeCountsSize;
    int errorCodeCountsCount;
    xmllintStageCount *errorStageCounts;
    int errorStageCountsSize;
    int errorStageCountsCount;
    xmlTime errorStatsStart;

    xmlChar *paths[MAX_PATHS + 1];
    int nbpaths;

    xmlTime begin;
    xmlTime end;
} xmllintState;

static int xmllintMaxmem;
static int xmllintMaxmemReached;
static int xmllintOom;

static void
getTime(xmlTime *time);
static long long
xmllintTimeDiffMs(const xmlTime *start, const xmlTime *end);
static const char *
xmllintRedactValue(const xmllintState *lint, unsigned flag,
                   const char *value);

/************************************************************************
 *									*
 *		 Entity loading control and customization.		*
 *									*
 ************************************************************************/

static void
parsePath(xmllintState *lint, const xmlChar *path) {
    const xmlChar *cur;

    if (path == NULL)
	return;
    while (*path != 0) {
	if (lint->nbpaths >= MAX_PATHS) {
	    fprintf(lint->errStream, "MAX_PATHS reached: too many paths\n");
            lint->progresult = XMLLINT_ERR_UNCLASS;
	    return;
	}
	cur = path;
	while ((*cur == ' ') || (*cur == PATH_SEPARATOR))
	    cur++;
	path = cur;
	while ((*cur != 0) && (*cur != ' ') && (*cur != PATH_SEPARATOR))
	    cur++;
	if (cur != path) {
	    lint->paths[lint->nbpaths] = xmlStrndup(path, cur - path);
	    if (lint->paths[lint->nbpaths] != NULL)
		lint->nbpaths++;
	    path = cur;
	}
    }
}

static xmlParserErrors
xmllintResourceLoader(void *ctxt, const char *URL,
                      const char *ID, xmlResourceType type,
                      xmlParserInputFlags flags, xmlParserInputPtr *out) {
    xmllintState *lint = ctxt;
    xmlParserErrors code;
    int i;
    const char *lastsegment = URL;
    const char *iter = URL;

    if ((lint->nbpaths > 0) && (iter != NULL)) {
	while (*iter != 0) {
	    if (*iter == '/')
		lastsegment = iter + 1;
	    iter++;
	}
    }

    if (lint->defaultResourceLoader != NULL)
        code = lint->defaultResourceLoader(NULL, URL, ID, type, flags, out);
    else
        code = xmlNewInputFromUrl(URL, flags, out);
    if (code != XML_IO_ENOENT) {
        if ((lint->appOptions & XML_LINT_USE_LOAD_TRACE) && (code == XML_ERR_OK)) {
            fprintf(lint->errStream, "Loaded URL=\"%s\" ID=\"%s\"\n",
                    URL, ID ? ID : "(null)");
        }
        return(code);
    }

    for (i = 0; i < lint->nbpaths; i++) {
	xmlChar *newURL;

	newURL = xmlStrdup((const xmlChar *) lint->paths[i]);
	newURL = xmlStrcat(newURL, (const xmlChar *) "/");
	newURL = xmlStrcat(newURL, (const xmlChar *) lastsegment);
	if (newURL != NULL) {
            if (lint->defaultResourceLoader != NULL)
                code = lint->defaultResourceLoader(NULL, (const char *) newURL,
                                                   ID, type, flags, out);
            else
                code = xmlNewInputFromUrl((const char *) newURL, flags, out);
            if (code != XML_IO_ENOENT) {
                if ((lint->appOptions & XML_LINT_USE_LOAD_TRACE) && (code == XML_ERR_OK)) {
                    fprintf(lint->errStream, "Loaded URL=\"%s\" ID=\"%s\"\n",
                            newURL, ID ? ID : "(null)");
                }
	        xmlFree(newURL);
                return(code);
            }
	    xmlFree(newURL);
	}
    }

    return(XML_IO_ENOENT);
}

/************************************************************************
 *									*
 *		 	Error reporting helpers				*
 *									*
 ************************************************************************/

static void
xmllintJsonEscape(FILE *out, const char *str) {
    const unsigned char *cur;

    if (str == NULL) {
        fputs("null", out);
        return;
    }

    fputc('"', out);
    for (cur = (const unsigned char *) str; *cur != '\0'; cur++) {
        switch (*cur) {
            case '\"':
                fputs("\\\"", out);
                break;
            case '\\':
                fputs("\\\\", out);
                break;
            case '\b':
                fputs("\\b", out);
                break;
            case '\f':
                fputs("\\f", out);
                break;
            case '\n':
                fputs("\\n", out);
                break;
            case '\r':
                fputs("\\r", out);
                break;
            case '\t':
                fputs("\\t", out);
                break;
            default:
                if (*cur < 0x20) {
                    fprintf(out, "\\u%04x", *cur);
                } else {
                    fputc(*cur, out);
                }
        }
    }
    fputc('"', out);
}

static int
xmllintBufAdd(xmlBufferPtr buf, const char *str, int len) {
    if (len < 0)
        len = (int) strlen(str);
    return xmlBufferAdd(buf, BAD_CAST str, len);
}

static int
xmllintBufEscape(xmlBufferPtr buf, const char *str) {
    char tmp[7];
    const unsigned char *cur;

    if (str == NULL)
        return xmllintBufAdd(buf, "null", 4);

    if (xmllintBufAdd(buf, "\"", 1) != 0)
        return -1;

    for (cur = (const unsigned char *) str; *cur != '\0'; cur++) {
        switch (*cur) {
            case '\"':
                if (xmllintBufAdd(buf, "\\\"", 2) != 0)
                    return -1;
                break;
            case '\\':
                if (xmllintBufAdd(buf, "\\\\", 2) != 0)
                    return -1;
                break;
            case '\b':
                if (xmllintBufAdd(buf, "\\b", 2) != 0)
                    return -1;
                break;
            case '\f':
                if (xmllintBufAdd(buf, "\\f", 2) != 0)
                    return -1;
                break;
            case '\n':
                if (xmllintBufAdd(buf, "\\n", 2) != 0)
                    return -1;
                break;
            case '\r':
                if (xmllintBufAdd(buf, "\\r", 2) != 0)
                    return -1;
                break;
            case '\t':
                if (xmllintBufAdd(buf, "\\t", 2) != 0)
                    return -1;
                break;
            default:
                if (*cur < 0x20) {
                    snprintf(tmp, sizeof(tmp), "\\u%04x", *cur);
                    if (xmllintBufAdd(buf, tmp, 6) != 0)
                        return -1;
                } else {
                    if (xmlBufferAdd(buf, cur, 1) != 0)
                        return -1;
                }
        }
    }

    return xmllintBufAdd(buf, "\"", 1);
}

static int
xmllintBufAddInt(xmlBufferPtr buf, int value) {
    char tmp[32];
    int len = snprintf(tmp, sizeof(tmp), "%d", value);
    if (len <= 0)
        return -1;
    return xmllintBufAdd(buf, tmp, len);
}

static int
xmllintBufAddKey(xmlBufferPtr buf, const char *key) {
    if (xmllintBufEscape(buf, key) != 0)
        return -1;
    return xmllintBufAdd(buf, ":", 1);
}

static const char *
xmllintRedactValue(const xmllintState *lint, unsigned flag,
                   const char *value) {
    if ((lint == NULL) || (value == NULL))
        return value;
    if ((lint->errorRedactFlags & flag) == 0)
        return value;
    return "[redacted]";
}

static int
xmllintBufAddOptions(xmlBufferPtr buf, const char *key, int options,
                     const xmllintOptName *defs) {
    int i;
    int first = 1;

    if (xmllintBufAdd(buf, ",", 1) != 0)
        return -1;
    if (xmllintBufAddKey(buf, key) != 0)
        return -1;
    if (xmllintBufAdd(buf, "[", 1) != 0)
        return -1;
    for (i = 0; defs[i].name != NULL; i++) {
        if ((options & defs[i].flag) == 0)
            continue;
        if (!first) {
            if (xmllintBufAdd(buf, ",", 1) != 0)
                return -1;
        }
        if (xmllintBufEscape(buf, defs[i].name) != 0)
            return -1;
        first = 0;
    }
    return xmllintBufAdd(buf, "]", 1);
}

static int
xmllintBufAddChecksum(xmlBufferPtr buf, const char *key, int valid,
                      unsigned long long checksum) {
    if (xmllintBufAdd(buf, ",", 1) != 0)
        return -1;
    if (xmllintBufAddKey(buf, key) != 0)
        return -1;
    if (!valid)
        return xmllintBufAdd(buf, "null", 4);
    {
        char tmp[32];
        int len = snprintf(tmp, sizeof(tmp), "\"%016llx\"", checksum);
        if (len <= 0)
            return -1;
        return xmllintBufAdd(buf, tmp, len);
    }
}

static int
xmllintBufAddWindow(xmlBufferPtr buf, const char *key, xmllintState *lint) {
    xmlParserCtxtPtr ctxt;
    const xmlChar *start = NULL;
    int size = 0;
    int offset = 0;
    int len;
    char *tmp;

    if ((lint == NULL) || (lint->errorJsonWindow <= 0))
        return 0;

    if (lint->errorRedactFlags & XMLLINT_REDACT_WINDOW) {
        if (xmllintBufAdd(buf, ",", 1) != 0 ||
            xmllintBufAddKey(buf, key) != 0)
            return -1;
        return xmllintBufAdd(buf, "null", 4);
    }

    ctxt = lint->ctxt;
    if (ctxt == NULL)
        return 0;

    if (xmlCtxtGetInputWindow(ctxt, 0, &start, &size, &offset) < 0)
        return 0;
    if ((start == NULL) || (size <= 0))
        return 0;

    if (offset < 0)
        offset = 0;
    if (offset > size)
        offset = size;

    len = lint->errorJsonWindow;
    if (len > size - offset)
        len = size - offset;

    tmp = xmlMalloc((size_t) len + 1);
    if (tmp == NULL)
        return -1;
    memcpy(tmp, start + offset, (size_t) len);
    tmp[len] = '\0';

    if (xmllintBufAdd(buf, ",", 1) != 0 ||
        xmllintBufAddKey(buf, key) != 0 ||
        xmllintBufEscape(buf, tmp) != 0) {
        xmlFree(tmp);
        return -1;
    }
    xmlFree(tmp);
    return 0;
}

static unsigned long long
xmllintErrorFingerprint(const xmlError *error);

static int
xmllintJsonErrorToBuffer(xmlBufferPtr buf, xmllintState *lint,
                         const xmlError *error, const char *filename) {
    static const xmllintOptName xmlOpts[] = {
        { XML_PARSE_RECOVER, "RECOVER" },
        { XML_PARSE_NOENT, "NOENT" },
        { XML_PARSE_DTDLOAD, "DTDLOAD" },
        { XML_PARSE_DTDATTR, "DTDATTR" },
        { XML_PARSE_DTDVALID, "DTDVALID" },
        { XML_PARSE_NOERROR, "NOERROR" },
        { XML_PARSE_NOWARNING, "NOWARNING" },
        { XML_PARSE_PEDANTIC, "PEDANTIC" },
        { XML_PARSE_NOBLANKS, "NOBLANKS" },
        { XML_PARSE_SAX1, "SAX1" },
        { XML_PARSE_XINCLUDE, "XINCLUDE" },
        { XML_PARSE_NONET, "NONET" },
        { XML_PARSE_NODICT, "NODICT" },
        { XML_PARSE_NSCLEAN, "NSCLEAN" },
        { XML_PARSE_NOCDATA, "NOCDATA" },
        { XML_PARSE_NOXINCNODE, "NOXINCNODE" },
        { XML_PARSE_COMPACT, "COMPACT" },
        { XML_PARSE_OLD10, "OLDXML10" },
        { XML_PARSE_NOBASEFIX, "NOBASEFIX" },
        { XML_PARSE_HUGE, "HUGE" },
        { XML_PARSE_BIG_LINES, "BIG_LINES" },
        { XML_PARSE_NO_XXE, "NO_XXE" },
        { XML_PARSE_UNZIP, "UNZIP" },
        { XML_PARSE_NO_SYS_CATALOG, "NO_SYS_CATALOG" },
        { XML_PARSE_CATALOG_PI, "CATALOG_PI" },
        { XML_PARSE_SKIP_IDS, "SKIP_IDS" },
        { XML_PARSE_REQUIRE_LOADER, "REQUIRE_LOADER" },
        { 0, NULL }
    };
#ifdef LIBXML_HTML_ENABLED
    static const xmllintOptName htmlOpts[] = {
        { HTML_PARSE_RECOVER, "RECOVER" },
        { HTML_PARSE_NODEFDTD, "NODEFDTD" },
        { HTML_PARSE_NOERROR, "NOERROR" },
        { HTML_PARSE_NOWARNING, "NOWARNING" },
        { HTML_PARSE_PEDANTIC, "PEDANTIC" },
        { HTML_PARSE_NOBLANKS, "NOBLANKS" },
        { HTML_PARSE_NONET, "NONET" },
        { HTML_PARSE_NOIMPLIED, "NOIMPLIED" },
        { HTML_PARSE_COMPACT, "COMPACT" },
        { HTML_PARSE_HTML5, "HTML5" },
        { HTML_PARSE_BIG_LINES, "BIG_LINES" },
        { HTML_PARSE_IGNORE_ENC, "IGNORE_ENC" },
        { HTML_PARSE_HUGE, "HUGE" },
        { 0, NULL }
    };
#endif

    if (xmllintBufAdd(buf, "{", 1) != 0)
        return -1;
    if (xmllintBufAddKey(buf, "schema_version") != 0 ||
        xmllintBufAddInt(buf, XMLLINT_JSON_SCHEMA_VERSION) != 0)
        return -1;
    {
        const char *file = xmllintRedactValue(lint, XMLLINT_REDACT_FILE,
                                              filename);
        const char *message = xmllintRedactValue(lint, XMLLINT_REDACT_MESSAGE,
                                                 error->message);
        const char *str1 = xmllintRedactValue(lint, XMLLINT_REDACT_STR1,
                                              error->str1);
        const char *str2 = xmllintRedactValue(lint, XMLLINT_REDACT_STR2,
                                              error->str2);
        const char *str3 = xmllintRedactValue(lint, XMLLINT_REDACT_STR3,
                                              error->str3);
        const char *resourceType = str2;
        const char *stage = str3;

        if (xmllintBufAdd(buf, ",", 1) != 0)
            return -1;
        if (xmllintBufAddKey(buf, "file") != 0 ||
            xmllintBufEscape(buf, file) != 0)
            return -1;
        if (xmllintBufAdd(buf, ",\"domain\":", 10) != 0 ||
            xmllintBufAddInt(buf, error->domain) != 0)
            return -1;
        if (xmllintBufAdd(buf, ",\"code\":", 8) != 0 ||
            xmllintBufAddInt(buf, error->code) != 0)
            return -1;
        if (xmllintBufAdd(buf, ",\"level\":", 9) != 0 ||
            xmllintBufAddInt(buf, error->level) != 0)
            return -1;
        if (xmllintBufAdd(buf, ",\"line\":", 8) != 0 ||
            xmllintBufAddInt(buf, error->line) != 0)
            return -1;
        if (xmllintBufAdd(buf, ",\"column\":", 10) != 0 ||
            xmllintBufAddInt(buf, error->int2) != 0)
            return -1;
        if (xmllintBufAdd(buf, ",\"message\":", 11) != 0 ||
            xmllintBufEscape(buf, message) != 0)
            return -1;
        if (xmllintBufAdd(buf, ",\"str1\":", 8) != 0 ||
            xmllintBufEscape(buf, str1) != 0)
            return -1;
        if (xmllintBufAdd(buf, ",\"str2\":", 8) != 0 ||
            xmllintBufEscape(buf, str2) != 0)
            return -1;
        if (xmllintBufAdd(buf, ",\"str3\":", 8) != 0 ||
            xmllintBufEscape(buf, str3) != 0)
            return -1;
        if (xmllintBufAdd(buf, ",\"resource_type\":", 17) != 0 ||
            xmllintBufEscape(buf, resourceType) != 0)
            return -1;
        if (xmllintBufAdd(buf, ",\"stage\":", 9) != 0 ||
            xmllintBufEscape(buf, stage) != 0)
            return -1;
    }
    if (xmllintBufAddOptions(buf, "parse_options",
                             lint->parseOptions, xmlOpts) != 0)
        return -1;
#ifdef LIBXML_HTML_ENABLED
    if (lint->appOptions & XML_LINT_HTML_ENABLED) {
        if (xmllintBufAddOptions(buf, "html_options",
                                 lint->htmlOptions, htmlOpts) != 0)
            return -1;
    }
#endif
    if (xmllintBufAddWindow(buf, "window", lint) != 0)
        return -1;
    if (xmllintBufAddChecksum(buf, "checksum",
                              lint->checksumValid,
                              lint->errorChecksum) != 0)
        return -1;
    if (xmllintBufAdd(buf, ",\"fingerprint\":\"", 17) != 0)
        return -1;
    {
        char tmp[32];
        unsigned long long fp = xmllintErrorFingerprint(error);
        int len = snprintf(tmp, sizeof(tmp), "%016llx", fp);
        if (len <= 0)
            return -1;
        if (xmllintBufAdd(buf, tmp, len) != 0)
            return -1;
    }
    if (xmllintBufAdd(buf, "\"", 1) != 0)
        return -1;
    if (xmllintBufAdd(buf, "}", 1) != 0)
        return -1;
    return 0;
}

#ifndef _WIN32
static void
xmllintSyslogError(xmllintState *lint, const xmlError *error) {
    xmlBufferPtr buf;

    if ((lint == NULL) || (error == NULL))
        return;

    buf = xmlBufferCreate();
    if (buf == NULL)
        return;

    if (xmllintJsonErrorToBuffer(buf, lint, error,
                                 lint->errorJsonArrayFile) == 0) {
        syslog(LOG_ERR, "%s", (const char *) xmlBufferContent(buf));
    }

    xmlBufferFree(buf);
}
#endif
static void
xmllintJsonIndent(FILE *out, int pretty, int level) {
    int i;

    if (!pretty)
        return;
    fputc('\n', out);
    for (i = 0; i < level; i++)
        fputs("  ", out);
}

static void
xmllintJsonKey(FILE *out, int pretty, int level, const char *key) {
    xmllintJsonIndent(out, pretty, level);
    xmllintJsonEscape(out, key);
    fputs(pretty ? ": " : ":", out);
}

static void
xmllintJsonAddInt(FILE *out, int value) {
    fprintf(out, "%d", value);
}

static void
xmllintJsonAddLongLong(FILE *out, long long value) {
    fprintf(out, "%lld", value);
}

static void
xmllintJsonAddTime(FILE *out, int pretty, int level, const char *key) {
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0)
        return;

    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, key);
    fputs("{", out);
    xmllintJsonKey(out, pretty, level + 1, "sec");
    xmllintJsonAddInt(out, (int) tv.tv_sec);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level + 1, "usec");
    xmllintJsonAddInt(out, (int) tv.tv_usec);
    xmllintJsonIndent(out, pretty, level);
    fputs("}", out);
}

static const char *
xmllintDomainName(int domain);

static void
xmllintJsonAddChecksum(FILE *out, int pretty, int level,
                       const char *key, int valid,
                       unsigned long long checksum) {
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, key);
    if (!valid) {
        fputs("null", out);
    } else {
        fprintf(out, "\"%016llx\"", checksum);
    }
}

static unsigned long long
xmllintErrorFingerprint(const xmlError *error) {
    unsigned long long hash = 1469598103934665603ULL;
    const unsigned char *cur;
    int vals[3];
    int i;

    if (error == NULL)
        return 0;

    vals[0] = error->domain;
    vals[1] = error->code;
    vals[2] = error->level;
    for (i = 0; i < 3; i++) {
        hash ^= (unsigned char) (vals[i] & 0xFF);
        hash *= 1099511628211ULL;
        hash ^= (unsigned char) ((vals[i] >> 8) & 0xFF);
        hash *= 1099511628211ULL;
        hash ^= (unsigned char) ((vals[i] >> 16) & 0xFF);
        hash *= 1099511628211ULL;
        hash ^= (unsigned char) ((vals[i] >> 24) & 0xFF);
        hash *= 1099511628211ULL;
    }

    for (cur = (const unsigned char *) (error->message ? error->message : "");
         *cur != '\0'; cur++) {
        hash ^= *cur;
        hash *= 1099511628211ULL;
    }
    for (cur = (const unsigned char *) (error->str1 ? error->str1 : "");
         *cur != '\0'; cur++) {
        hash ^= *cur;
        hash *= 1099511628211ULL;
    }
    for (cur = (const unsigned char *) (error->str2 ? error->str2 : "");
         *cur != '\0'; cur++) {
        hash ^= *cur;
        hash *= 1099511628211ULL;
    }
    for (cur = (const unsigned char *) (error->str3 ? error->str3 : "");
         *cur != '\0'; cur++) {
        hash ^= *cur;
        hash *= 1099511628211ULL;
    }

    return hash;
}

static void
xmllintJsonAddOptions(FILE *out, int pretty, int level,
                      const char *key, int options,
                      const xmllintOptName *defs) {
    int first = 1;
    int i;

    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, key);
    fputs("[", out);
    for (i = 0; defs[i].name != NULL; i++) {
        if ((options & defs[i].flag) == 0)
            continue;
        if (!first)
            fputc(',', out);
        xmllintJsonIndent(out, pretty, level + 1);
        xmllintJsonEscape(out, defs[i].name);
        first = 0;
    }
    if (!first)
        xmllintJsonIndent(out, pretty, level);
    fputs("]", out);
}

static void
xmllintJsonAddWindow(FILE *out, int pretty, int level,
                     const char *key, xmllintState *lint) {
    xmlParserCtxtPtr ctxt;
    const xmlChar *start = NULL;
    int size = 0;
    int offset = 0;
    int len;
    char *buf;

    if ((lint == NULL) || (lint->errorJsonWindow <= 0))
        return;

    if (lint->errorRedactFlags & XMLLINT_REDACT_WINDOW) {
        fputc(pretty ? ',' : ',', out);
        xmllintJsonKey(out, pretty, level, key);
        fputs("null", out);
        return;
    }

    ctxt = lint->ctxt;
    if (ctxt == NULL)
        return;

    if (xmlCtxtGetInputWindow(ctxt, 0, &start, &size, &offset) < 0)
        return;

    if ((start == NULL) || (size <= 0))
        return;

    if (offset < 0)
        offset = 0;
    if (offset > size)
        offset = size;

    len = lint->errorJsonWindow;
    if (len > size - offset)
        len = size - offset;

    buf = xmlMalloc((size_t) len + 1);
    if (buf == NULL)
        return;
    memcpy(buf, start + offset, (size_t) len);
    buf[len] = '\0';

    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, key);
    xmllintJsonEscape(out, buf);
    xmlFree(buf);
}

static void
xmllintJsonAddFingerprint(FILE *out, int pretty, int level,
                          const char *key, const xmlError *error) {
    unsigned long long fp = xmllintErrorFingerprint(error);

    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, key);
    fprintf(out, "\"%016llx\"", fp);
}

static void
xmllintErrorStatsReset(xmllintState *lint) {
    int i;

    if (lint == NULL)
        return;
    memset(lint->errorDomainCounts, 0, sizeof(lint->errorDomainCounts));
    memset(lint->errorLevelCounts, 0, sizeof(lint->errorLevelCounts));
    lint->errorCodeCountsCount = 0;
    for (i = 0; i < lint->errorStageCountsCount; i++) {
        xmlFree(lint->errorStageCounts[i].name);
        lint->errorStageCounts[i].name = NULL;
    }
    lint->errorStageCountsCount = 0;
    getTime(&lint->errorStatsStart);
}

static void
xmllintErrorStageAdd(xmllintState *lint, const xmlError *error) {
    const char *stage;
    xmlTime now;
    long long msec;
    int i;

    if ((lint == NULL) || (error == NULL))
        return;

    stage = error->str3 ? error->str3 : "unknown";
    getTime(&now);
    msec = xmllintTimeDiffMs(&lint->errorStatsStart, &now);

    for (i = 0; i < lint->errorStageCountsCount; i++) {
        if (strcmp(lint->errorStageCounts[i].name, stage) == 0) {
            lint->errorStageCounts[i].count += 1;
            lint->errorStageCounts[i].lastMs = msec;
            return;
        }
    }

    if (lint->errorStageCountsCount == lint->errorStageCountsSize) {
        int newSize = lint->errorStageCountsSize ? lint->errorStageCountsSize * 2 : 8;
        xmllintStageCount *tmp = xmlRealloc(lint->errorStageCounts,
                                            sizeof(*lint->errorStageCounts) *
                                            newSize);
        if (tmp == NULL)
            return;
        lint->errorStageCounts = tmp;
        lint->errorStageCountsSize = newSize;
    }

    lint->errorStageCounts[lint->errorStageCountsCount].name =
        xmlMemStrdup(stage);
    if (lint->errorStageCounts[lint->errorStageCountsCount].name == NULL)
        return;
    lint->errorStageCounts[lint->errorStageCountsCount].count = 1;
    lint->errorStageCounts[lint->errorStageCountsCount].firstMs = msec;
    lint->errorStageCounts[lint->errorStageCountsCount].lastMs = msec;
    lint->errorStageCountsCount += 1;
}

static void
xmllintErrorStatsAdd(xmllintState *lint, const xmlError *error) {
    int i;

    if ((lint == NULL) || (error == NULL))
        return;

    if ((error->domain >= 0) && (error->domain <= XML_FROM_URI))
        lint->errorDomainCounts[error->domain] += 1;

    if ((error->level >= 0) && (error->level <= XML_ERR_FATAL))
        lint->errorLevelCounts[error->level] += 1;

    for (i = 0; i < lint->errorCodeCountsCount; i++) {
        if (lint->errorCodeCounts[i].code == error->code) {
            lint->errorCodeCounts[i].count += 1;
            return;
        }
    }

    if (lint->errorCodeCountsCount == lint->errorCodeCountsSize) {
        int newSize = lint->errorCodeCountsSize ? lint->errorCodeCountsSize * 2 : 16;
        xmllintCodeCount *tmp = xmlRealloc(lint->errorCodeCounts,
                                           sizeof(*lint->errorCodeCounts) * newSize);
        if (tmp == NULL)
            return;
        lint->errorCodeCounts = tmp;
        lint->errorCodeCountsSize = newSize;
    }

    lint->errorCodeCounts[lint->errorCodeCountsCount].code = error->code;
    lint->errorCodeCounts[lint->errorCodeCountsCount].count = 1;
    lint->errorCodeCountsCount += 1;

    xmllintErrorStageAdd(lint, error);
}

static void
xmllintJsonWriteSummary(FILE *out, int pretty, int level, xmllintState *lint) {
    int i;
    int first;

    if ((lint == NULL) || (!lint->errorJsonSummary))
        return;

    fputs("{", out);

    xmllintJsonKey(out, pretty, level + 1, "by_level");
    fputs("{", out);
    first = 1;
    for (i = 0; i <= XML_ERR_FATAL; i++) {
        if (!first)
            fputc(',', out);
        xmllintJsonKey(out, pretty, level + 2,
                       i == XML_ERR_NONE ? "none" :
                       i == XML_ERR_WARNING ? "warning" :
                       i == XML_ERR_ERROR ? "error" : "fatal");
        xmllintJsonAddInt(out, lint->errorLevelCounts[i]);
        first = 0;
    }
    xmllintJsonIndent(out, pretty, level + 1);
    fputs("}", out);

    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level + 1, "by_domain");
    fputs("{", out);
    first = 1;
    for (i = 0; i <= XML_FROM_URI; i++) {
        if (lint->errorDomainCounts[i] == 0)
            continue;
        if (!first)
            fputc(',', out);
        xmllintJsonKey(out, pretty, level + 2, xmllintDomainName(i));
        xmllintJsonAddInt(out, lint->errorDomainCounts[i]);
        first = 0;
    }
    xmllintJsonIndent(out, pretty, level + 1);
    fputs("}", out);

    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level + 1, "by_code");
    fputs("{", out);
    first = 1;
    for (i = 0; i < lint->errorCodeCountsCount; i++) {
        char keyBuf[32];
        snprintf(keyBuf, sizeof(keyBuf), "%d",
                 lint->errorCodeCounts[i].code);
        if (!first)
            fputc(',', out);
        xmllintJsonKey(out, pretty, level + 2, keyBuf);
        xmllintJsonAddInt(out, lint->errorCodeCounts[i].count);
        first = 0;
    }
    xmllintJsonIndent(out, pretty, level + 1);
    fputs("}", out);

    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level + 1, "by_stage");
    fputs("{", out);
    first = 1;
    for (i = 0; i < lint->errorStageCountsCount; i++) {
        long long span;

        if (!first)
            fputc(',', out);
        xmllintJsonKey(out, pretty, level + 2,
                       lint->errorStageCounts[i].name);
        fputs("{", out);
        xmllintJsonKey(out, pretty, level + 3, "count");
        xmllintJsonAddInt(out, lint->errorStageCounts[i].count);
        fputc(pretty ? ',' : ',', out);
        xmllintJsonKey(out, pretty, level + 3, "first_ms");
        xmllintJsonAddLongLong(out, lint->errorStageCounts[i].firstMs);
        fputc(pretty ? ',' : ',', out);
        xmllintJsonKey(out, pretty, level + 3, "last_ms");
        xmllintJsonAddLongLong(out, lint->errorStageCounts[i].lastMs);
        span = lint->errorStageCounts[i].lastMs -
               lint->errorStageCounts[i].firstMs;
        fputc(pretty ? ',' : ',', out);
        xmllintJsonKey(out, pretty, level + 3, "span_ms");
        xmllintJsonAddLongLong(out, span < 0 ? 0 : span);
        xmllintJsonIndent(out, pretty, level + 2);
        fputs("}", out);
        first = 0;
    }
    xmllintJsonIndent(out, pretty, level + 1);
    fputs("}", out);

    xmllintJsonIndent(out, pretty, level);
    fputs("}", out);
}

static void
xmllintWriteXmlError(FILE *out, xmllintState *lint,
                     const xmlError *error, const char *filename) {
    xmlError tmp;
    xmlChar *xml = NULL;
    int len = 0;
    const char *file;

    if ((out == NULL) || (lint == NULL) || (error == NULL))
        return;

    file = filename ? filename : error->file;

    tmp = *error;
    tmp.file = (char *) xmllintRedactValue(lint, XMLLINT_REDACT_FILE, file);
    tmp.message = (char *) xmllintRedactValue(lint, XMLLINT_REDACT_MESSAGE,
                                              error->message);
    tmp.str1 = (char *) xmllintRedactValue(lint, XMLLINT_REDACT_STR1,
                                           error->str1);
    tmp.str2 = (char *) xmllintRedactValue(lint, XMLLINT_REDACT_STR2,
                                           error->str2);
    tmp.str3 = (char *) xmllintRedactValue(lint, XMLLINT_REDACT_STR3,
                                           error->str3);

    if ((xmlErrorToXml(&tmp, &xml, &len) == 0) && (xml != NULL)) {
        fwrite(xml, 1, (size_t) len, out);
        fputc('\n', out);
    }
    xmlFree(xml);
}

static const char *
xmllintDomainName(int domain) {
    switch (domain) {
        case XML_FROM_NONE: return "none";
        case XML_FROM_PARSER: return "parser";
        case XML_FROM_TREE: return "tree";
        case XML_FROM_NAMESPACE: return "namespace";
        case XML_FROM_DTD: return "dtd";
        case XML_FROM_HTML: return "html";
        case XML_FROM_MEMORY: return "memory";
        case XML_FROM_OUTPUT: return "output";
        case XML_FROM_IO: return "io";
        case XML_FROM_FTP: return "ftp";
        case XML_FROM_HTTP: return "http";
        case XML_FROM_XINCLUDE: return "xinclude";
        case XML_FROM_XPATH: return "xpath";
        case XML_FROM_XPOINTER: return "xpointer";
        case XML_FROM_REGEXP: return "regexp";
        case XML_FROM_DATATYPE: return "datatype";
        case XML_FROM_SCHEMASP: return "schemasp";
        case XML_FROM_SCHEMASV: return "schemasv";
        case XML_FROM_RELAXNGP: return "relaxngp";
        case XML_FROM_RELAXNGV: return "relaxngv";
        case XML_FROM_CATALOG: return "catalog";
        case XML_FROM_C14N: return "c14n";
        case XML_FROM_XSLT: return "xslt";
        case XML_FROM_VALID: return "valid";
        case XML_FROM_CHECK: return "check";
        case XML_FROM_WRITER: return "writer";
        case XML_FROM_MODULE: return "module";
        case XML_FROM_I18N: return "i18n";
        case XML_FROM_SCHEMATRONV: return "schematronv";
        case XML_FROM_BUFFER: return "buffer";
        case XML_FROM_URI: return "uri";
        default: return "unknown";
    }
}

static unsigned long long
xmllintChecksumFile(const char *filename, int *ok) {
    FILE *fp;
    unsigned long long hash = 1469598103934665603ULL;
    int c;

    *ok = 0;
    if ((filename == NULL) || (strcmp(filename, "-") == 0))
        return 0;

    fp = fopen(filename, "rb");
    if (fp == NULL)
        return 0;

    while ((c = fgetc(fp)) != EOF) {
        hash ^= (unsigned char) c;
        hash *= 1099511628211ULL;
    }

    fclose(fp);
    *ok = 1;
    return hash;
}

static void
xmllintWriteJsonError(FILE *out, xmllintState *lint,
                      const xmlError *error, const char *filename) {
    static const xmllintOptName xmlOpts[] = {
        { XML_PARSE_RECOVER, "RECOVER" },
        { XML_PARSE_NOENT, "NOENT" },
        { XML_PARSE_DTDLOAD, "DTDLOAD" },
        { XML_PARSE_DTDATTR, "DTDATTR" },
        { XML_PARSE_DTDVALID, "DTDVALID" },
        { XML_PARSE_NOERROR, "NOERROR" },
        { XML_PARSE_NOWARNING, "NOWARNING" },
        { XML_PARSE_PEDANTIC, "PEDANTIC" },
        { XML_PARSE_NOBLANKS, "NOBLANKS" },
        { XML_PARSE_SAX1, "SAX1" },
        { XML_PARSE_XINCLUDE, "XINCLUDE" },
        { XML_PARSE_NONET, "NONET" },
        { XML_PARSE_NODICT, "NODICT" },
        { XML_PARSE_NSCLEAN, "NSCLEAN" },
        { XML_PARSE_NOCDATA, "NOCDATA" },
        { XML_PARSE_NOXINCNODE, "NOXINCNODE" },
        { XML_PARSE_COMPACT, "COMPACT" },
        { XML_PARSE_OLD10, "OLDXML10" },
        { XML_PARSE_NOBASEFIX, "NOBASEFIX" },
        { XML_PARSE_HUGE, "HUGE" },
        { XML_PARSE_BIG_LINES, "BIG_LINES" },
        { XML_PARSE_NO_XXE, "NO_XXE" },
        { XML_PARSE_UNZIP, "UNZIP" },
        { XML_PARSE_NO_SYS_CATALOG, "NO_SYS_CATALOG" },
        { XML_PARSE_CATALOG_PI, "CATALOG_PI" },
        { XML_PARSE_SKIP_IDS, "SKIP_IDS" },
        { XML_PARSE_REQUIRE_LOADER, "REQUIRE_LOADER" },
        { 0, NULL }
    };
#ifdef LIBXML_HTML_ENABLED
    static const xmllintOptName htmlOpts[] = {
        { HTML_PARSE_RECOVER, "RECOVER" },
        { HTML_PARSE_NODEFDTD, "NODEFDTD" },
        { HTML_PARSE_NOERROR, "NOERROR" },
        { HTML_PARSE_NOWARNING, "NOWARNING" },
        { HTML_PARSE_PEDANTIC, "PEDANTIC" },
        { HTML_PARSE_NOBLANKS, "NOBLANKS" },
        { HTML_PARSE_NONET, "NONET" },
        { HTML_PARSE_NOIMPLIED, "NOIMPLIED" },
        { HTML_PARSE_COMPACT, "COMPACT" },
        { HTML_PARSE_HTML5, "HTML5" },
        { HTML_PARSE_BIG_LINES, "BIG_LINES" },
        { HTML_PARSE_IGNORE_ENC, "IGNORE_ENC" },
        { HTML_PARSE_HUGE, "HUGE" },
        { 0, NULL }
    };
#endif
    int pretty = lint->errorJsonPretty;
    int level = pretty ? 1 : 0;
    const char *file = xmllintRedactValue(lint, XMLLINT_REDACT_FILE, filename);
    const char *message = xmllintRedactValue(lint, XMLLINT_REDACT_MESSAGE,
                                             error->message);
    const char *str1 = xmllintRedactValue(lint, XMLLINT_REDACT_STR1,
                                          error->str1);
    const char *str2 = xmllintRedactValue(lint, XMLLINT_REDACT_STR2,
                                          error->str2);
    const char *str3 = xmllintRedactValue(lint, XMLLINT_REDACT_STR3,
                                          error->str3);
    const char *resourceType = str2;
    const char *stage = str3;

    fputs("{", out);
    xmllintJsonKey(out, pretty, level, "schema_version");
    xmllintJsonAddInt(out, XMLLINT_JSON_SCHEMA_VERSION);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "file");
    xmllintJsonEscape(out, file);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "domain");
    xmllintJsonAddInt(out, error->domain);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "code");
    xmllintJsonAddInt(out, error->code);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "level");
    xmllintJsonAddInt(out, error->level);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "line");
    xmllintJsonAddInt(out, error->line);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "column");
    xmllintJsonAddInt(out, error->int2);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "message");
    xmllintJsonEscape(out, message);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "str1");
    xmllintJsonEscape(out, str1);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "str2");
    xmllintJsonEscape(out, str2);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "str3");
    xmllintJsonEscape(out, str3);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "resource_type");
    xmllintJsonEscape(out, resourceType);
    fputc(pretty ? ',' : ',', out);
    xmllintJsonKey(out, pretty, level, "stage");
    xmllintJsonEscape(out, stage);
    xmllintJsonAddOptions(out, pretty, level, "parse_options",
                          lint->parseOptions, xmlOpts);
#ifdef LIBXML_HTML_ENABLED
    if (lint->appOptions & XML_LINT_HTML_ENABLED) {
        xmllintJsonAddOptions(out, pretty, level, "html_options",
                              lint->htmlOptions, htmlOpts);
    }
#endif
    xmllintJsonAddWindow(out, pretty, level, "window", lint);
    xmllintJsonAddChecksum(out, pretty, level, "checksum",
                           lint->checksumValid, lint->errorChecksum);
    xmllintJsonAddFingerprint(out, pretty, level, "fingerprint", error);
    xmllintJsonAddTime(out, pretty, level, "timestamp");
    xmllintJsonIndent(out, pretty, 0);
    fputs("}", out);
}

static void
xmllintStructuredError(void *ctx, const xmlError *error) {
    xmllintState *lint = ctx;
    FILE *out;
    int emitJson;
    int emitSyslog;
    int emitXml;

    if ((lint == NULL) || (error == NULL))
        return;

    xmllintErrorStatsAdd(lint, error);

    emitJson = lint->errorJson;
    emitSyslog = lint->errorSyslog;
    emitXml = lint->errorXml;

    if ((lint->errorJsonLimit > 0) &&
        (lint->errorJsonCount >= lint->errorJsonLimit)) {
        emitJson = 0;
        emitSyslog = 0;
    }

    if (!emitJson && !emitSyslog && !emitXml)
        return;

    if (emitJson) {
        if ((error->level == XML_ERR_WARNING) &&
            (lint->errorJsonWarnStream != NULL)) {
            out = lint->errorJsonWarnStream;
        } else {
            out = lint->errorJsonStream ? lint->errorJsonStream
                                        : lint->errStream;
        }

        if (lint->errorJsonArray) {
            if (lint->errorJsonArrayCount > 0)
                fputc(',', out);
            xmllintWriteJsonError(out, lint, error, lint->errorJsonArrayFile);
            lint->errorJsonArrayCount += 1;
        } else {
            xmllintWriteJsonError(out, lint, error, lint->errorJsonArrayFile);
            fputc('\n', out);
        }

        lint->errorJsonCount += 1;
    }

    if (emitXml) {
        FILE *xmlOut = lint->errorXmlStream ? lint->errorXmlStream
                                            : lint->errStream;
        xmllintWriteXmlError(xmlOut, lint, error, lint->errorJsonArrayFile);
    }

#ifndef _WIN32
    if (emitSyslog)
        xmllintSyslogError(lint, error);
#endif
}

static void
xmllintDumpErrorRing(xmllintState *lint, xmlParserCtxtPtr ctxt,
                     const char *filename) {
    xmlError *errors;
    int count;
    int allocCount;
    int i;

    if ((lint == NULL) || (ctxt == NULL))
        return;
    if (lint->errorRingSize <= 0)
        return;

    count = xmlCtxtGetErrorRing(ctxt, NULL, 0);
    if (count < 0)
        return;

    allocCount = (count > 0) ? count : 1;
    errors = xmlMalloc(sizeof(*errors) * allocCount);
    if (errors == NULL)
        return;
    memset(errors, 0, sizeof(*errors) * allocCount);

    if (count > 0) {
        if (xmlCtxtGetErrorRing(ctxt, errors, count) < 0) {
            xmlFree(errors);
            return;
        }
    }

    {
        FILE *out = lint->errorRingDumpStream ? lint->errorRingDumpStream :
                                                lint->errStream;
    fputs("{\"schema_version\":", out);
    xmllintJsonAddInt(out, XMLLINT_JSON_SCHEMA_VERSION);
    fputs(",\"file\":", out);
    xmllintJsonEscape(out, xmllintRedactValue(lint, XMLLINT_REDACT_FILE,
                                             filename));
    fputs(",\"errors\":[", out);
        for (i = 0; i < count; i++) {
            if (i > 0)
                fputc(',', out);
            xmllintWriteJsonError(out, lint, &errors[i], filename);
        }
    fputs("]}\n", out);
    }

    for (i = 0; i < count; i++)
        xmlResetError(&errors[i]);
    xmlFree(errors);
}

static int
xmllintWriteBinHeader(FILE *out) {
    static const unsigned char header[] = { 'X', 'E', 'R', 'B', 0, 0, 0, 1 };

    if (fwrite(header, 1, sizeof(header), out) != sizeof(header))
        return -1;
    return 0;
}

static int
xmllintWriteBinRecord(FILE *out, const unsigned char *data, unsigned int len) {
    unsigned char hdr[4];

    hdr[0] = (unsigned char) ((len >> 24) & 0xFF);
    hdr[1] = (unsigned char) ((len >> 16) & 0xFF);
    hdr[2] = (unsigned char) ((len >> 8) & 0xFF);
    hdr[3] = (unsigned char) (len & 0xFF);

    if (fwrite(hdr, 1, sizeof(hdr), out) != sizeof(hdr))
        return -1;
    if (len > 0 && fwrite(data, 1, len, out) != len)
        return -1;

    return 0;
}

static void
xmllintDumpErrorRingBinary(xmllintState *lint, xmlParserCtxtPtr ctxt,
                           const char *filename) {
    xmlError *errors;
    xmlBufferPtr buf;
    int count;
    int allocCount;
    int i;
    FILE *out;

    if ((lint == NULL) || (ctxt == NULL))
        return;

    if (lint->errorRingDumpBinStream == NULL)
        return;

    if (lint->errorRingSize <= 0)
        return;

    count = xmlCtxtGetErrorRing(ctxt, NULL, 0);
    if (count < 0)
        return;

    allocCount = (count > 0) ? count : 1;
    errors = xmlMalloc(sizeof(*errors) * allocCount);
    if (errors == NULL)
        return;
    memset(errors, 0, sizeof(*errors) * allocCount);

    if (count > 0) {
        if (xmlCtxtGetErrorRing(ctxt, errors, count) < 0) {
            xmlFree(errors);
            return;
        }
    }

    buf = xmlBufferCreate();
    if (buf == NULL) {
        for (i = 0; i < count; i++)
            xmlResetError(&errors[i]);
        xmlFree(errors);
        return;
    }

    if (xmllintBufAdd(buf, "{", 1) == 0 &&
        xmllintBufAddKey(buf, "schema_version") == 0 &&
        xmllintBufAddInt(buf, XMLLINT_JSON_SCHEMA_VERSION) == 0 &&
        xmllintBufAdd(buf, ",", 1) == 0 &&
        xmllintBufAddKey(buf, "file") == 0 &&
        xmllintBufEscape(buf,
                         xmllintRedactValue(lint, XMLLINT_REDACT_FILE,
                                            filename)) == 0 &&
        xmllintBufAdd(buf, ",\"errors\":[", 11) == 0) {
        for (i = 0; i < count; i++) {
            if (i > 0)
                xmllintBufAdd(buf, ",", 1);
            if (xmllintJsonErrorToBuffer(buf, lint, &errors[i], filename) != 0)
                break;
        }
        if (i == count) {
            xmllintBufAdd(buf, "]}", 2);
        }
    }

    out = lint->errorRingDumpBinStream;
    if (xmlBufferContent(buf) != NULL) {
        xmllintWriteBinRecord(out,
                              (const unsigned char *) xmlBufferContent(buf),
                              (unsigned int) xmlBufferLength(buf));
    }

    xmlBufferFree(buf);

    for (i = 0; i < count; i++)
        xmlResetError(&errors[i]);
    xmlFree(errors);
}

static void
xmllintCborWriteType(FILE *out, unsigned int major, unsigned long long val) {
    if (val < 24) {
        fputc((int) ((major << 5) | val), out);
    } else if (val <= 0xFF) {
        fputc((int) ((major << 5) | 24), out);
        fputc((int) val, out);
    } else if (val <= 0xFFFF) {
        fputc((int) ((major << 5) | 25), out);
        fputc((int) ((val >> 8) & 0xFF), out);
        fputc((int) (val & 0xFF), out);
    } else if (val <= 0xFFFFFFFFULL) {
        fputc((int) ((major << 5) | 26), out);
        fputc((int) ((val >> 24) & 0xFF), out);
        fputc((int) ((val >> 16) & 0xFF), out);
        fputc((int) ((val >> 8) & 0xFF), out);
        fputc((int) (val & 0xFF), out);
    } else {
        fputc((int) ((major << 5) | 27), out);
        fputc((int) ((val >> 56) & 0xFF), out);
        fputc((int) ((val >> 48) & 0xFF), out);
        fputc((int) ((val >> 40) & 0xFF), out);
        fputc((int) ((val >> 32) & 0xFF), out);
        fputc((int) ((val >> 24) & 0xFF), out);
        fputc((int) ((val >> 16) & 0xFF), out);
        fputc((int) ((val >> 8) & 0xFF), out);
        fputc((int) (val & 0xFF), out);
    }
}

static void
xmllintCborWriteText(FILE *out, const char *str) {
    size_t len = str ? strlen(str) : 0;

    xmllintCborWriteType(out, 3, (unsigned long long) len);
    if (len > 0)
        fwrite(str, 1, len, out);
}

static void
xmllintDumpErrorRingCbor(xmllintState *lint, xmlParserCtxtPtr ctxt,
                         const char *filename) {
    xmlError *errors;
    int count;
    int allocCount;
    int i;
    FILE *out;

    if ((lint == NULL) || (ctxt == NULL))
        return;

    if (lint->errorRingDumpCborStream == NULL)
        return;

    count = xmlCtxtGetErrorRing(ctxt, NULL, 0);
    if (count < 0)
        return;

    allocCount = (count > 0) ? count : 1;
    errors = xmlMalloc(sizeof(*errors) * allocCount);
    if (errors == NULL)
        return;
    memset(errors, 0, sizeof(*errors) * allocCount);

    if (count > 0) {
        if (xmlCtxtGetErrorRing(ctxt, errors, count) < 0) {
            xmlFree(errors);
            return;
        }
    }

    out = lint->errorRingDumpCborStream;
    xmllintCborWriteType(out, 5, 2); /* map with 2 keys */
    xmllintCborWriteText(out, "file");
    xmllintCborWriteText(out,
                         xmllintRedactValue(lint, XMLLINT_REDACT_FILE,
                                            filename ? filename : ""));
    xmllintCborWriteText(out, "errors");
    xmllintCborWriteType(out, 4, (unsigned long long) count);
    for (i = 0; i < count; i++) {
        const char *message = xmllintRedactValue(lint, XMLLINT_REDACT_MESSAGE,
                                                 errors[i].message);
        const char *str1 = xmllintRedactValue(lint, XMLLINT_REDACT_STR1,
                                              errors[i].str1);
        const char *str2 = xmllintRedactValue(lint, XMLLINT_REDACT_STR2,
                                              errors[i].str2);
        const char *str3 = xmllintRedactValue(lint, XMLLINT_REDACT_STR3,
                                              errors[i].str3);

        xmllintCborWriteType(out, 5, 8); /* map with 8 keys */
        xmllintCborWriteText(out, "domain");
        xmllintCborWriteType(out, 0, (unsigned long long) errors[i].domain);
        xmllintCborWriteText(out, "code");
        xmllintCborWriteType(out, 0, (unsigned long long) errors[i].code);
        xmllintCborWriteText(out, "level");
        xmllintCborWriteType(out, 0, (unsigned long long) errors[i].level);
        xmllintCborWriteText(out, "line");
        xmllintCborWriteType(out, 0, (unsigned long long) errors[i].line);
        xmllintCborWriteText(out, "column");
        xmllintCborWriteType(out, 0, (unsigned long long) errors[i].int2);
        xmllintCborWriteText(out, "message");
        xmllintCborWriteText(out, message);
        xmllintCborWriteText(out, "str1");
        xmllintCborWriteText(out, str1);
        xmllintCborWriteText(out, "str2");
        xmllintCborWriteText(out, str2);
        xmllintCborWriteText(out, "str3");
        xmllintCborWriteText(out, str3);
    }

    for (i = 0; i < count; i++)
        xmlResetError(&errors[i]);
    xmlFree(errors);
}

static void
xmllintStartErrorArray(xmllintState *lint, const char *filename) {
    FILE *out;

    if ((lint == NULL) || (!lint->errorJsonArray))
        return;

    out = lint->errorJsonStream ? lint->errorJsonStream : lint->errStream;
    fputs("{\"schema_version\":", out);
    xmllintJsonAddInt(out, XMLLINT_JSON_SCHEMA_VERSION);
    fputs(",\"file\":", out);
    xmllintJsonEscape(out, xmllintRedactValue(lint, XMLLINT_REDACT_FILE,
                                             filename));
    fputs(",\"errors\":[", out);
    lint->errorJsonArrayOpen = 1;
    lint->errorJsonArrayCount = 0;
}

static void
xmllintFinishErrorArray(xmllintState *lint) {
    FILE *out;

    if ((lint == NULL) || (!lint->errorJsonArrayOpen))
        return;

    out = lint->errorJsonStream ? lint->errorJsonStream : lint->errStream;
    fputs("]", out);
    if (lint->errorJsonSummary)
        xmllintJsonWriteSummary(out, lint->errorJsonPretty ? 1 : 0,
                                lint->errorJsonPretty ? 1 : 0, lint);
    if (lint->errorJsonChecksum)
        xmllintJsonAddChecksum(out, lint->errorJsonPretty ? 1 : 0,
                               lint->errorJsonPretty ? 1 : 0,
                               "checksum",
                               lint->checksumValid,
                               lint->errorChecksum);
    fputs("}\n", out);
    lint->errorJsonArrayOpen = 0;
}

static void
xmllintWriteSummaryLine(xmllintState *lint, const char *filename) {
    FILE *out;

    if ((lint == NULL) || (!lint->errorJsonSummary))
        return;

    out = lint->errorJsonStream ? lint->errorJsonStream : lint->errStream;
    fputs("{\"schema_version\":", out);
    xmllintJsonAddInt(out, XMLLINT_JSON_SCHEMA_VERSION);
    fputs(",\"file\":", out);
    xmllintJsonEscape(out, xmllintRedactValue(lint, XMLLINT_REDACT_FILE,
                                             filename));
    fputs(",\"summary\":", out);
    xmllintJsonWriteSummary(out, 0, 0, lint);
    if (lint->errorJsonChecksum)
        xmllintJsonAddChecksum(out, 0, 0, "checksum",
                               lint->checksumValid, lint->errorChecksum);
    fputs("}\n", out);
}

/************************************************************************
 *									*
 *		 	Core parsing functions				*
 *									*
 ************************************************************************/

#ifdef LIBXML_ZLIB_ENABLED
static int
xmllintGzRead(void *ctxt, char *buf, int len) {
    return gzread(ctxt, buf, len);
}

#ifdef LIBXML_OUTPUT_ENABLED
static int
xmllintGzWrite(void *ctxt, const char *buf, int len) {
    return gzwrite(ctxt, buf, len);
}
#endif

static int
xmllintGzClose(void *ctxt) {
    if (gzclose(ctxt) != Z_OK)
        return -1;

    return 0;
}
#endif

static xmlDocPtr
parseXml(xmllintState *lint, const char *filename) {
    xmlParserCtxtPtr ctxt = lint->ctxt;
    xmlDocPtr doc;
#ifdef LIBXML_ZLIB_ENABLED
    gzFile gz;
#endif

#ifdef LIBXML_PUSH_ENABLED
    if (lint->appOptions & XML_LINT_PUSH_ENABLED) {
        FILE *f;
        int res;
        char chars[4096];

        if ((filename[0] == '-') && (filename[1] == 0)) {
            f = stdin;
        } else {
            f = fopen(filename, "rb");
            if (f == NULL) {
                fprintf(lint->errStream, "Can't open %s\n", filename);
                lint->progresult = XMLLINT_ERR_RDFILE;
                return(NULL);
            }
        }

        while ((res = fread(chars, 1, 4096, f)) > 0) {
            xmlParseChunk(ctxt, chars, res, 0);
        }
        xmlParseChunk(ctxt, chars, 0, 1);
        doc = xmlCtxtGetDocument(ctxt);

        if (f != stdin)
            fclose(f);

        return(doc);
    }
#endif /* LIBXML_PUSH_ENABLED */

#if HAVE_DECL_MMAP
    if (lint->appOptions & XML_LINT_MEMORY) {
        xmlParserInputPtr input;

        input = xmlNewInputFromMemory(filename,
                                      lint->memoryData, lint->memorySize,
                                      XML_INPUT_BUF_STATIC);
        if (input == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            return(NULL);
        }
        doc = xmlCtxtParseDocument(ctxt, input);
        return(doc);
    }
#endif

#ifdef LIBXML_ZLIB_ENABLED
    if (strcmp(filename, "-") == 0)
        gz = gzdopen(STDIN_FILENO, "rb");
    else
        gz = gzopen(filename, "rb");

    if (gz == NULL) {
        fprintf(lint->errStream, "Can't open %s\n", filename);
        lint->progresult = XMLLINT_ERR_RDFILE;
        return(NULL);
    }

    doc = xmlCtxtReadIO(ctxt, xmllintGzRead, xmllintGzClose, gz,
                        filename, NULL, lint->parseOptions);
#else
    if (strcmp(filename, "-") == 0)
        doc = xmlCtxtReadFd(ctxt, STDIN_FILENO, "-", NULL, lint->parseOptions);
    else
        doc = xmlCtxtReadFile(ctxt, filename, NULL, lint->parseOptions);
#endif

    return(doc);
}

#ifdef LIBXML_HTML_ENABLED
static xmlDocPtr
parseHtml(xmllintState *lint, const char *filename) {
    xmlParserCtxtPtr ctxt = lint->ctxt;
    xmlDocPtr doc;

#ifdef LIBXML_PUSH_ENABLED
    if (lint->appOptions & XML_LINT_PUSH_ENABLED) {
        FILE *f;
        int res;
        char chars[4096];

        if ((filename[0] == '-') && (filename[1] == 0)) {
            f = stdin;
        } else {
	    f = fopen(filename, "rb");
            if (f == NULL) {
                fprintf(lint->errStream, "Can't open %s\n", filename);
                lint->progresult = XMLLINT_ERR_RDFILE;
                return(NULL);
            }
        }

        while ((res = fread(chars, 1, 4096, f)) > 0) {
            htmlParseChunk(ctxt, chars, res, 0);
        }
        htmlParseChunk(ctxt, chars, 0, 1);
        doc = xmlCtxtGetDocument(ctxt);

        if (f != stdin)
            fclose(f);

        return(doc);
    }
#endif /* LIBXML_PUSH_ENABLED */

#if HAVE_DECL_MMAP
    if (lint->appOptions & XML_LINT_MEMORY) {
        xmlParserInputPtr input;

        input = xmlNewInputFromMemory(filename,
                                      lint->memoryData, lint->memorySize,
                                      XML_INPUT_BUF_STATIC);
        if (input == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            return(NULL);
        }
        doc = htmlCtxtParseDocument(ctxt, input);
        return(doc);
    }
#endif

    if (strcmp(filename, "-") == 0)
        doc = htmlCtxtReadFd(ctxt, STDIN_FILENO, "-", NULL,
                             lint->htmlOptions);
    else
        doc = htmlCtxtReadFile(ctxt, filename, NULL, lint->htmlOptions);

    return(doc);
}
#endif /* LIBXML_HTML_ENABLED */

/************************************************************************
 *									*
 * Memory allocation consumption debugging				*
 *									*
 ************************************************************************/

#define XMLLINT_ABORT_ON_FAILURE 0

static void
myFreeFunc(void *mem) {
    xmlMemFree(mem);
}

static void *
myMallocFunc(size_t size) {
    void *ret;

    if (xmlMemUsed() + size > (size_t) xmllintMaxmem) {
#if XMLLINT_ABORT_ON_FAILURE
        abort();
#endif
        xmllintMaxmemReached = 1;
        xmllintOom = 1;
        return(NULL);
    }

    ret = xmlMemMalloc(size);
    if (ret == NULL)
        xmllintOom = 1;

    return(ret);
}

static void *
myReallocFunc(void *mem, size_t size) {
    void *ret;
    size_t oldsize = xmlMemSize(mem);

    if (xmlMemUsed() + size - oldsize > (size_t) xmllintMaxmem) {
#if XMLLINT_ABORT_ON_FAILURE
        abort();
#endif
        xmllintMaxmemReached = 1;
        xmllintOom = 1;
        return(NULL);
    }

    ret = xmlMemRealloc(mem, size);
    if (ret == NULL)
        xmllintOom = 1;

    return(ret);
}

static char *
myStrdupFunc(const char *str) {
    size_t size;
    char *ret;

    if (str == NULL)
        return(NULL);

    size = strlen(str) + 1;
    if (xmlMemUsed() + size > (size_t) xmllintMaxmem) {
#if XMLLINT_ABORT_ON_FAILURE
        abort();
#endif
        xmllintMaxmemReached = 1;
        xmllintOom = 1;
        return(NULL);
    }

    ret = xmlMemMalloc(size);
    if (ret == NULL) {
        xmllintOom = 1;
        return(NULL);
    }

    memcpy(ret, str, size);

    return(ret);
}

/************************************************************************
 *									*
 * Internal timing routines to remove the necessity to have		*
 * unix-specific function calls.					*
 *									*
 ************************************************************************/

static void
getTime(xmlTime *time) {
#ifdef _WIN32
    struct __timeb64 timebuffer;

    _ftime64(&timebuffer);
    time->sec = timebuffer.time;
    time->usec = timebuffer.millitm * 1000;
#else /* _WIN32 */
    struct timeval tv;

    gettimeofday(&tv, NULL);
    time->sec = tv.tv_sec;
    time->usec = tv.tv_usec;
#endif /* _WIN32 */
}

static long long
xmllintTimeDiffMs(const xmlTime *start, const xmlTime *end) {
    long long msec;

    if ((start == NULL) || (end == NULL))
        return 0;

    msec = (long long) (end->sec - start->sec);
    msec *= 1000;
    msec += (long long) ((end->usec - start->usec) / 1000);

    return msec;
}

/*
 * startTimer: call where you want to start timing
 */
static void
startTimer(xmllintState *lint)
{
    getTime(&lint->begin);
}

/*
 * endTimer: call where you want to stop timing and to print out a
 *           message about the timing performed; format is a printf
 *           type argument
 */
static void LIBXML_ATTR_FORMAT(2,3)
endTimer(xmllintState *lint, const char *fmt, ...)
{
    xmlSeconds msec;
    va_list ap;

    getTime(&lint->end);
    msec = lint->end.sec - lint->begin.sec;
    msec *= 1000;
    msec += (lint->end.usec - lint->begin.usec) / 1000;

    va_start(ap, fmt);
    vfprintf(lint->errStream, fmt, ap);
    va_end(ap);

    fprintf(lint->errStream, " took %ld ms\n", (long) msec);
}

/************************************************************************
 *									*
 *			SAX based tests					*
 *									*
 ************************************************************************/

/*
 * empty SAX block
 */
static const xmlSAXHandler emptySAXHandler = {
    NULL, /* internalSubset */
    NULL, /* isStandalone */
    NULL, /* hasInternalSubset */
    NULL, /* hasExternalSubset */
    NULL, /* resolveEntity */
    NULL, /* getEntity */
    NULL, /* entityDecl */
    NULL, /* notationDecl */
    NULL, /* attributeDecl */
    NULL, /* elementDecl */
    NULL, /* unparsedEntityDecl */
    NULL, /* setDocumentLocator */
    NULL, /* startDocument */
    NULL, /* endDocument */
    NULL, /* startElement */
    NULL, /* endElement */
    NULL, /* reference */
    NULL, /* characters */
    NULL, /* ignorableWhitespace */
    NULL, /* processingInstruction */
    NULL, /* comment */
    NULL, /* xmlParserWarning */
    NULL, /* xmlParserError */
    NULL, /* xmlParserError */
    NULL, /* getParameterEntity */
    NULL, /* cdataBlock; */
    NULL, /* externalSubset; */
    XML_SAX2_MAGIC,
    NULL,
    NULL, /* startElementNs */
    NULL, /* endElementNs */
    NULL  /* xmlStructuredErrorFunc */
};

static int
isStandaloneDebug(void *ctx)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return(0);
    fprintf(stdout, "SAX.isStandalone()\n");
    return(0);
}

static int
hasInternalSubsetDebug(void *ctx)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return(0);
    fprintf(stdout, "SAX.hasInternalSubset()\n");
    return(0);
}

static int
hasExternalSubsetDebug(void *ctx)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return(0);
    fprintf(stdout, "SAX.hasExternalSubset()\n");
    return(0);
}

static void
internalSubsetDebug(void *ctx, const xmlChar *name,
	       const xmlChar *ExternalID, const xmlChar *SystemID)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.internalSubset(%s,", name);
    if (ExternalID == NULL)
	fprintf(stdout, " ,");
    else
	fprintf(stdout, " %s,", ExternalID);
    if (SystemID == NULL)
	fprintf(stdout, " )\n");
    else
	fprintf(stdout, " %s)\n", SystemID);
}

static void
externalSubsetDebug(void *ctx, const xmlChar *name,
	       const xmlChar *ExternalID, const xmlChar *SystemID)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.externalSubset(%s,", name);
    if (ExternalID == NULL)
	fprintf(stdout, " ,");
    else
	fprintf(stdout, " %s,", ExternalID);
    if (SystemID == NULL)
	fprintf(stdout, " )\n");
    else
	fprintf(stdout, " %s)\n", SystemID);
}

static xmlParserInputPtr
resolveEntityDebug(void *ctx, const xmlChar *publicId, const xmlChar *systemId)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return(NULL);
    /* xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr) ctx; */


    fprintf(stdout, "SAX.resolveEntity(");
    if (publicId != NULL)
	fprintf(stdout, "%s", (char *)publicId);
    else
	fprintf(stdout, " ");
    if (systemId != NULL)
	fprintf(stdout, ", %s)\n", (char *)systemId);
    else
	fprintf(stdout, ", )\n");
    return(NULL);
}

static xmlEntityPtr
getEntityDebug(void *ctx, const xmlChar *name)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return(NULL);
    fprintf(stdout, "SAX.getEntity(%s)\n", name);
    return(NULL);
}

static xmlEntityPtr
getParameterEntityDebug(void *ctx, const xmlChar *name)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return(NULL);
    fprintf(stdout, "SAX.getParameterEntity(%s)\n", name);
    return(NULL);
}

static void
entityDeclDebug(void *ctx, const xmlChar *name, int type,
          const xmlChar *publicId, const xmlChar *systemId, xmlChar *content)
{
    xmllintState *lint = ctx;
    const xmlChar *nullstr = BAD_CAST "(null)";

    /* not all libraries handle printing null pointers nicely */
    if (publicId == NULL)
        publicId = nullstr;
    if (systemId == NULL)
        systemId = nullstr;
    if (content == NULL)
        content = (xmlChar *)nullstr;
    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.entityDecl(%s, %d, %s, %s, %s)\n",
            name, type, publicId, systemId, content);
}

static void
attributeDeclDebug(void *ctx, const xmlChar * elem,
                   const xmlChar * name, int type, int def,
                   const xmlChar * defaultValue, xmlEnumerationPtr tree)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
        return;
    if (defaultValue == NULL)
        fprintf(stdout, "SAX.attributeDecl(%s, %s, %d, %d, NULL, ...)\n",
                elem, name, type, def);
    else
        fprintf(stdout, "SAX.attributeDecl(%s, %s, %d, %d, %s, ...)\n",
                elem, name, type, def, defaultValue);
    xmlFreeEnumeration(tree);
}

static void
elementDeclDebug(void *ctx, const xmlChar *name, int type,
	    xmlElementContentPtr content ATTRIBUTE_UNUSED)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.elementDecl(%s, %d, ...)\n",
            name, type);
}

static void
notationDeclDebug(void *ctx, const xmlChar *name,
	     const xmlChar *publicId, const xmlChar *systemId)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.notationDecl(%s, %s, %s)\n",
            (char *) name, (char *) publicId, (char *) systemId);
}

static void
unparsedEntityDeclDebug(void *ctx, const xmlChar *name,
		   const xmlChar *publicId, const xmlChar *systemId,
		   const xmlChar *notationName)
{
    xmllintState *lint = ctx;
    const xmlChar *nullstr = BAD_CAST "(null)";

    if (publicId == NULL)
        publicId = nullstr;
    if (systemId == NULL)
        systemId = nullstr;
    if (notationName == NULL)
        notationName = nullstr;
    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.unparsedEntityDecl(%s, %s, %s, %s)\n",
            (char *) name, (char *) publicId, (char *) systemId,
	    (char *) notationName);
}

static void
setDocumentLocatorDebug(void *ctx, xmlSAXLocatorPtr loc ATTRIBUTE_UNUSED)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.setDocumentLocator()\n");
}

static void
startDocumentDebug(void *ctx)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.startDocument()\n");
}

static void
endDocumentDebug(void *ctx)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.endDocument()\n");
}

static void
startElementDebug(void *ctx, const xmlChar *name, const xmlChar **atts)
{
    xmllintState *lint = ctx;
    int i;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.startElement(%s", (char *) name);
    if (atts != NULL) {
        for (i = 0;(atts[i] != NULL);i++) {
	    fprintf(stdout, ", %s='", atts[i++]);
	    if (atts[i] != NULL)
	        fprintf(stdout, "%s'", atts[i]);
	}
    }
    fprintf(stdout, ")\n");
}

static void
endElementDebug(void *ctx, const xmlChar *name)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.endElement(%s)\n", (char *) name);
}

static void
charactersDebug(void *ctx, const xmlChar *ch, int len)
{
    xmllintState *lint = ctx;
    char out[40];
    int i;

    lint->callbacks++;
    if (lint->noout)
	return;
    for (i = 0;(i<len) && (i < 30);i++)
	out[i] = (char) ch[i];
    out[i] = 0;

    fprintf(stdout, "SAX.characters(%s, %d)\n", out, len);
}

static void
referenceDebug(void *ctx, const xmlChar *name)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.reference(%s)\n", name);
}

static void
ignorableWhitespaceDebug(void *ctx, const xmlChar *ch, int len)
{
    xmllintState *lint = ctx;
    char out[40];
    int i;

    lint->callbacks++;
    if (lint->noout)
	return;
    for (i = 0;(i<len) && (i < 30);i++)
	out[i] = ch[i];
    out[i] = 0;
    fprintf(stdout, "SAX.ignorableWhitespace(%s, %d)\n", out, len);
}

static void
processingInstructionDebug(void *ctx, const xmlChar *target,
                      const xmlChar *data)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    if (data != NULL)
	fprintf(stdout, "SAX.processingInstruction(%s, %s)\n",
		(char *) target, (char *) data);
    else
	fprintf(stdout, "SAX.processingInstruction(%s, NULL)\n",
		(char *) target);
}

static void
cdataBlockDebug(void *ctx, const xmlChar *value, int len)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.pcdata(%.20s, %d)\n",
	    (char *) value, len);
}

static void
commentDebug(void *ctx, const xmlChar *value)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.comment(%s)\n", value);
}

static void LIBXML_ATTR_FORMAT(2,3)
warningDebug(void *ctx, const char *msg, ...)
{
    xmllintState *lint = ctx;
    va_list args;

    lint->callbacks++;
    if (lint->noout)
	return;
    va_start(args, msg);
    fprintf(stdout, "SAX.warning: ");
    vfprintf(stdout, msg, args);
    va_end(args);
}

static void LIBXML_ATTR_FORMAT(2,3)
errorDebug(void *ctx, const char *msg, ...)
{
    xmllintState *lint = ctx;
    va_list args;

    lint->callbacks++;
    if (lint->noout)
	return;
    va_start(args, msg);
    fprintf(stdout, "SAX.error: ");
    vfprintf(stdout, msg, args);
    va_end(args);
}

static void LIBXML_ATTR_FORMAT(2,3)
fatalErrorDebug(void *ctx, const char *msg, ...)
{
    xmllintState *lint = ctx;
    va_list args;

    lint->callbacks++;
    if (lint->noout)
	return;
    va_start(args, msg);
    fprintf(stdout, "SAX.fatalError: ");
    vfprintf(stdout, msg, args);
    va_end(args);
}

#ifdef LIBXML_SAX1_ENABLED
static const xmlSAXHandler debugSAXHandler = {
    internalSubsetDebug,
    isStandaloneDebug,
    hasInternalSubsetDebug,
    hasExternalSubsetDebug,
    resolveEntityDebug,
    getEntityDebug,
    entityDeclDebug,
    notationDeclDebug,
    attributeDeclDebug,
    elementDeclDebug,
    unparsedEntityDeclDebug,
    setDocumentLocatorDebug,
    startDocumentDebug,
    endDocumentDebug,
    startElementDebug,
    endElementDebug,
    referenceDebug,
    charactersDebug,
    ignorableWhitespaceDebug,
    processingInstructionDebug,
    commentDebug,
    warningDebug,
    errorDebug,
    fatalErrorDebug,
    getParameterEntityDebug,
    cdataBlockDebug,
    externalSubsetDebug,
    1,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif

/*
 * SAX2 specific callbacks
 */

static void
startElementNsDebug(void *ctx,
                    const xmlChar *localname,
                    const xmlChar *prefix,
                    const xmlChar *URI,
		    int nb_namespaces,
		    const xmlChar **namespaces,
		    int nb_attributes,
		    int nb_defaulted,
		    const xmlChar **attributes)
{
    xmllintState *lint = ctx;
    int i;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.startElementNs(%s", (char *) localname);
    if (prefix == NULL)
	fprintf(stdout, ", NULL");
    else
	fprintf(stdout, ", %s", (char *) prefix);
    if (URI == NULL)
	fprintf(stdout, ", NULL");
    else
	fprintf(stdout, ", '%s'", (char *) URI);
    fprintf(stdout, ", %d", nb_namespaces);

    if (namespaces != NULL) {
        for (i = 0;i < nb_namespaces * 2;i++) {
	    fprintf(stdout, ", xmlns");
	    if (namespaces[i] != NULL)
	        fprintf(stdout, ":%s", namespaces[i]);
	    i++;
	    fprintf(stdout, "='%s'", namespaces[i]);
	}
    }
    fprintf(stdout, ", %d, %d", nb_attributes, nb_defaulted);
    if (attributes != NULL) {
        for (i = 0;i < nb_attributes * 5;i += 5) {
	    if (attributes[i + 1] != NULL)
		fprintf(stdout, ", %s:%s='", attributes[i + 1], attributes[i]);
	    else
		fprintf(stdout, ", %s='", attributes[i]);
	    fprintf(stdout, "%.4s...', %d", attributes[i + 3],
		    (int)(attributes[i + 4] - attributes[i + 3]));
	}
    }
    fprintf(stdout, ")\n");
}

static void
endElementNsDebug(void *ctx,
                  const xmlChar *localname,
                  const xmlChar *prefix,
                  const xmlChar *URI)
{
    xmllintState *lint = ctx;

    lint->callbacks++;
    if (lint->noout)
	return;
    fprintf(stdout, "SAX.endElementNs(%s", (char *) localname);
    if (prefix == NULL)
	fprintf(stdout, ", NULL");
    else
	fprintf(stdout, ", %s", (char *) prefix);
    if (URI == NULL)
	fprintf(stdout, ", NULL)\n");
    else
	fprintf(stdout, ", '%s')\n", (char *) URI);
}

static const xmlSAXHandler debugSAX2Handler = {
    internalSubsetDebug,
    isStandaloneDebug,
    hasInternalSubsetDebug,
    hasExternalSubsetDebug,
    resolveEntityDebug,
    getEntityDebug,
    entityDeclDebug,
    notationDeclDebug,
    attributeDeclDebug,
    elementDeclDebug,
    unparsedEntityDeclDebug,
    setDocumentLocatorDebug,
    startDocumentDebug,
    endDocumentDebug,
    startElementDebug, /* for HTML */
    endElementDebug,
    referenceDebug,
    charactersDebug,
    ignorableWhitespaceDebug,
    processingInstructionDebug,
    commentDebug,
    warningDebug,
    errorDebug,
    fatalErrorDebug,
    getParameterEntityDebug,
    cdataBlockDebug,
    externalSubsetDebug,
    XML_SAX2_MAGIC,
    NULL,
    startElementNsDebug,
    endElementNsDebug,
    NULL
};

static void
testSAX(xmllintState *lint, const char *filename) {
    lint->callbacks = 0;

#ifdef LIBXML_SCHEMAS_ENABLED
    if (lint->wxschemas != NULL) {
        int ret;
	xmlSchemaValidCtxtPtr vctxt;
        xmlParserInputBufferPtr buf;

        if (strcmp(filename, "-") == 0)
            buf = xmlParserInputBufferCreateFd(STDIN_FILENO,
                    XML_CHAR_ENCODING_NONE);
        else
            buf = xmlParserInputBufferCreateFilename(filename,
                    XML_CHAR_ENCODING_NONE);
        if (buf == NULL)
            return;

	vctxt = xmlSchemaNewValidCtxt(lint->wxschemas);
        if (vctxt == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            xmlFreeParserInputBuffer(buf);
            return;
        }
	xmlSchemaValidateSetFilename(vctxt, filename);

	ret = xmlSchemaValidateStream(vctxt, buf, 0, lint->ctxt->sax, lint);
	if (lint->repeat == 1) {
	    if (ret == 0) {
	        if ((lint->appOptions & XML_LINT_QUIET) != XML_LINT_QUIET) {
	            fprintf(lint->errStream, "%s validates\n", filename);
	        }
	    } else if (ret > 0) {
		fprintf(lint->errStream, "%s fails to validate\n", filename);
		lint->progresult = XMLLINT_ERR_VALID;
	    } else {
		fprintf(lint->errStream, "%s validation generated an internal error\n",
		       filename);
		lint->progresult = XMLLINT_ERR_VALID;
	    }
	}
	xmlSchemaFreeValidCtxt(vctxt);
    } else
#endif
#ifdef LIBXML_HTML_ENABLED
    if (lint->appOptions & XML_LINT_HTML_ENABLED) {
        parseHtml(lint, filename);
    } else
#endif
    {
        parseXml(lint, filename);
    }
}

/************************************************************************
 *									*
 *			Stream Test processing				*
 *									*
 ************************************************************************/
#ifdef LIBXML_READER_ENABLED
static void processNode(xmllintState *lint, xmlTextReaderPtr reader) {
    const xmlChar *name, *value;
    int type, empty;

    type = xmlTextReaderNodeType(reader);
    empty = xmlTextReaderIsEmptyElement(reader);

    if (lint->appOptions & XML_LINT_DEBUG_ENABLED) {
	name = xmlTextReaderConstName(reader);
	if (name == NULL)
	    name = BAD_CAST "--";

	value = xmlTextReaderConstValue(reader);


	printf("%d %d %s %d %d",
		xmlTextReaderDepth(reader),
		type,
		name,
		empty,
		xmlTextReaderHasValue(reader));
	if (value == NULL)
	    printf("\n");
	else {
	    printf(" %s\n", value);
	}
    }
#ifdef LIBXML_PATTERN_ENABLED
    if (lint->patternc) {
        xmlChar *path = NULL;
        int match = -1;

	if (type == XML_READER_TYPE_ELEMENT) {
	    /* do the check only on element start */
	    match = xmlPatternMatch(lint->patternc,
                                    xmlTextReaderCurrentNode(reader));

	    if (match) {
		path = xmlGetNodePath(xmlTextReaderCurrentNode(reader));
		printf("Node %s matches pattern %s\n", path, lint->pattern);
	    }
	}
	if (lint->patstream != NULL) {
	    int ret;

	    if (type == XML_READER_TYPE_ELEMENT) {
		ret = xmlStreamPush(lint->patstream,
		                    xmlTextReaderConstLocalName(reader),
				    xmlTextReaderConstNamespaceUri(reader));
		if (ret < 0) {
		    fprintf(lint->errStream, "xmlStreamPush() failure\n");
                    xmlFreeStreamCtxt(lint->patstream);
		    lint->patstream = NULL;
		} else if (ret != match) {
		    if (path == NULL) {
		        path = xmlGetNodePath(
		                       xmlTextReaderCurrentNode(reader));
		    }
		    fprintf(lint->errStream,
		            "xmlPatternMatch and xmlStreamPush disagree\n");
                    if (path != NULL)
                        fprintf(lint->errStream, "  pattern %s node %s\n",
                                lint->pattern, path);
                    else
		        fprintf(lint->errStream, "  pattern %s node %s\n",
			    lint->pattern, xmlTextReaderConstName(reader));
		}

	    }
	    if ((type == XML_READER_TYPE_END_ELEMENT) ||
	        ((type == XML_READER_TYPE_ELEMENT) && (empty))) {
	        ret = xmlStreamPop(lint->patstream);
		if (ret < 0) {
		    fprintf(lint->errStream, "xmlStreamPop() failure\n");
                    xmlFreeStreamCtxt(lint->patstream);
		    lint->patstream = NULL;
		}
	    }
	}
	if (path != NULL)
	    xmlFree(path);
    }
#endif
}

static void streamFile(xmllintState *lint, const char *filename) {
    xmlParserInputBufferPtr input = NULL;
    FILE *errStream = lint->errStream;
    xmlTextReaderPtr reader;
    int ret;

#if HAVE_DECL_MMAP
    if (lint->appOptions & XML_LINT_MEMORY) {
	reader = xmlReaderForMemory(lint->memoryData, lint->memorySize,
                                    filename, NULL, lint->parseOptions);
        if (reader == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            return;
        }
    } else
#endif
    {
#ifdef LIBXML_ZLIB_ENABLED
        gzFile gz;
#endif

        xmlResetLastError();

#ifdef LIBXML_ZLIB_ENABLED
        if (strcmp(filename, "-") == 0)
            gz = gzdopen(STDIN_FILENO, "rb");
        else
            gz = gzopen(filename, "rb");

        if (gz == NULL) {
            fprintf(lint->errStream, "Can't open %s\n", filename);
            lint->progresult = XMLLINT_ERR_RDFILE;
            return;
        }

        reader = xmlReaderForIO(xmllintGzRead, xmllintGzClose, gz,
                                filename, NULL, lint->parseOptions);
#else
        if (strcmp(filename, "-") == 0) {
            reader = xmlReaderForFd(STDIN_FILENO, "-", NULL,
                                    lint->parseOptions);
        }
        else {
            reader = xmlReaderForFile(filename, NULL, lint->parseOptions);
        }
#endif
        if (reader == NULL) {
            const xmlError *error = xmlGetLastError();

            if ((error != NULL) && (error->code == XML_ERR_NO_MEMORY)) {
                lint->progresult = XMLLINT_ERR_MEM;
            } else {
                fprintf(errStream, "Unable to open %s\n", filename);
                lint->progresult = XMLLINT_ERR_RDFILE;
            }
            return;
        }
    }

#ifdef LIBXML_PATTERN_ENABLED
    if (lint->patternc != NULL) {
        lint->patstream = xmlPatternGetStreamCtxt(lint->patternc);
	if (lint->patstream != NULL) {
	    ret = xmlStreamPush(lint->patstream, NULL, NULL);
	    if (ret < 0) {
		fprintf(errStream, "xmlStreamPush() failure\n");
		xmlFreeStreamCtxt(lint->patstream);
		lint->patstream = NULL;
            }
	}
    }
#endif


    xmlTextReaderSetResourceLoader(reader, xmllintResourceLoader, lint);
    if (lint->maxAmpl > 0)
        xmlTextReaderSetMaxAmplification(reader, lint->maxAmpl);

#ifdef LIBXML_RELAXNG_ENABLED
    if (lint->relaxng != NULL) {
        if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
            startTimer(lint);
        }
        ret = xmlTextReaderRelaxNGValidate(reader, lint->relaxng);
        if (ret < 0) {
            fprintf(errStream, "Relax-NG schema %s failed to compile\n",
                    lint->relaxng);
            lint->progresult = XMLLINT_ERR_SCHEMACOMP;
            lint->relaxng = NULL;
        }
        if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
            endTimer(lint, "Compiling the schemas");
        }
    }
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
    if (lint->schema != NULL) {
        if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
            startTimer(lint);
        }
        ret = xmlTextReaderSchemaValidate(reader, lint->schema);
        if (ret < 0) {
            fprintf(errStream, "XSD schema %s failed to compile\n",
                    lint->schema);
            lint->progresult = XMLLINT_ERR_SCHEMACOMP;
            lint->schema = NULL;
        }
        if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
            endTimer(lint, "Compiling the schemas");
        }
    }
#endif

    /*
     * Process all nodes in sequence
     */
    if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
        startTimer(lint);
    }
    ret = xmlTextReaderRead(reader);
    while (ret == 1) {
        if ((lint->appOptions & XML_LINT_DEBUG_ENABLED)
#ifdef LIBXML_PATTERN_ENABLED
            || (lint->patternc)
#endif
           )
            processNode(lint, reader);
        ret = xmlTextReaderRead(reader);
    }
    if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
#ifdef LIBXML_RELAXNG_ENABLED
        if (lint->relaxng != NULL)
            endTimer(lint, "Parsing and validating");
        else
#endif
#ifdef LIBXML_VALID_ENABLED
        if (lint->parseOptions & XML_PARSE_DTDVALID)
            endTimer(lint, "Parsing and validating");
        else
#endif
        endTimer(lint, "Parsing");
    }

#ifdef LIBXML_VALID_ENABLED
    if (lint->parseOptions & XML_PARSE_DTDVALID) {
        if (xmlTextReaderIsValid(reader) != 1) {
            fprintf(errStream,
                    "Document %s does not validate\n", filename);
            lint->progresult = XMLLINT_ERR_VALID;
        }
    }
#endif /* LIBXML_VALID_ENABLED */
#if defined(LIBXML_RELAXNG_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
    {
        int hasSchema = 0;

#ifdef LIBXML_RELAXNG_ENABLED
        if (lint->relaxng != NULL)
            hasSchema = 1;
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
        if (lint->schema != NULL)
            hasSchema = 1;
#endif
        if (hasSchema) {
            if (xmlTextReaderIsValid(reader) != 1) {
                fprintf(errStream, "%s fails to validate\n", filename);
                lint->progresult = XMLLINT_ERR_VALID;
            } else {
                if ((lint->appOptions & XML_LINT_QUIET) != XML_LINT_QUIET) {
                    fprintf(errStream, "%s validates\n", filename);
                }
            }
        }
    }
#endif
    /*
     * Done, cleanup and status
     */
    xmlFreeTextReader(reader);
    xmlFreeParserInputBuffer(input);
    if (ret != 0) {
        fprintf(errStream, "%s : failed to parse\n", filename);
        lint->progresult = XMLLINT_ERR_UNCLASS;
    }
#ifdef LIBXML_PATTERN_ENABLED
    if (lint->patstream != NULL) {
	xmlFreeStreamCtxt(lint->patstream);
	lint->patstream = NULL;
    }
#endif
}

static void walkDoc(xmllintState *lint, xmlDocPtr doc) {
    FILE *errStream = lint->errStream;
    xmlTextReaderPtr reader;
    int ret;

#ifdef LIBXML_PATTERN_ENABLED
    if (lint->pattern != NULL) {
        xmlNodePtr root;
        const xmlChar *namespaces[22];
        int i;
        xmlNsPtr ns;

        root = xmlDocGetRootElement(doc);
        if (root == NULL ) {
            fprintf(errStream,
                    "Document does not have a root element");
            lint->progresult = XMLLINT_ERR_UNCLASS;
            return;
        }
        for (ns = root->nsDef, i = 0;ns != NULL && i < 20;ns=ns->next) {
            namespaces[i++] = ns->href;
            namespaces[i++] = ns->prefix;
        }
        namespaces[i++] = NULL;
        namespaces[i] = NULL;

        ret = xmlPatternCompileSafe((const xmlChar *) lint->pattern, doc->dict,
                                    0, &namespaces[0], &lint->patternc);
	if (lint->patternc == NULL) {
            if (ret < 0) {
                lint->progresult = XMLLINT_ERR_MEM;
            } else {
                fprintf(errStream, "Pattern %s failed to compile\n",
                        lint->pattern);
                lint->progresult = XMLLINT_ERR_SCHEMAPAT;
            }
            goto error;
	}

        lint->patstream = xmlPatternGetStreamCtxt(lint->patternc);
        if (lint->patstream == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            goto error;
        }

        ret = xmlStreamPush(lint->patstream, NULL, NULL);
        if (ret < 0) {
            fprintf(errStream, "xmlStreamPush() failure\n");
            lint->progresult = XMLLINT_ERR_MEM;
            goto error;
        }
    }
#endif /* LIBXML_PATTERN_ENABLED */
    reader = xmlReaderWalker(doc);
    if (reader != NULL) {
	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    startTimer(lint);
	}
	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
	    if ((lint->appOptions & XML_LINT_DEBUG_ENABLED)
#ifdef LIBXML_PATTERN_ENABLED
	        || (lint->patternc)
#endif
	       )
		processNode(lint, reader);
	    ret = xmlTextReaderRead(reader);
	}
	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    endTimer(lint, "walking through the doc");
	}
	xmlFreeTextReader(reader);
	if (ret != 0) {
	    fprintf(errStream, "failed to walk through the doc\n");
	    lint->progresult = XMLLINT_ERR_UNCLASS;
	}
    } else {
	fprintf(errStream, "Failed to create a reader from the document\n");
	lint->progresult = XMLLINT_ERR_UNCLASS;
    }

#ifdef LIBXML_PATTERN_ENABLED
error:
    if (lint->patternc != NULL) {
        xmlFreePattern(lint->patternc);
        lint->patternc = NULL;
    }
    if (lint->patstream != NULL) {
	xmlFreeStreamCtxt(lint->patstream);
	lint->patstream = NULL;
    }
#endif
}
#endif /* LIBXML_READER_ENABLED */

#ifdef LIBXML_XPATH_ENABLED
/************************************************************************
 *									*
 *			XPath Query                                     *
 *									*
 ************************************************************************/

static void
doXPathDump(xmllintState *lint, xmlXPathObjectPtr cur) {
    switch(cur->type) {
        case XPATH_NODESET: {
#ifdef LIBXML_OUTPUT_ENABLED
            xmlOutputBufferPtr buf;
            xmlNodePtr node;
            int i;

            if ((cur->nodesetval == NULL) || (cur->nodesetval->nodeNr <= 0)) {
                lint->progresult = XMLLINT_ERR_XPATH_EMPTY;
                if ((lint->appOptions & XML_LINT_QUIET) != XML_LINT_QUIET) {
                    fprintf(lint->errStream, "XPath set is empty\n");
                }
                break;
            }
            buf = xmlOutputBufferCreateFile(stdout, NULL);
            if (buf == NULL) {
                lint->progresult = XMLLINT_ERR_MEM;
                return;
            }
            for (i = 0;i < cur->nodesetval->nodeNr;i++) {
                node = cur->nodesetval->nodeTab[i];
                xmlNodeDumpOutput(buf, NULL, node, 0, 0, NULL);
                xmlOutputBufferWrite(buf, 1, lint->xpathsep);
            }
            xmlOutputBufferClose(buf);
#else
            printf("xpath returned %d nodes\n", cur->nodesetval->nodeNr);
#endif
	    break;
        }
        case XPATH_BOOLEAN:
	    if (cur->boolval) printf("true%s", lint->xpathsep);
	    else printf("false%s", lint->xpathsep);
	    break;
        case XPATH_NUMBER:
	    switch (xmlXPathIsInf(cur->floatval)) {
	    case 1:
		printf("Infinity%s", lint->xpathsep);
		break;
	    case -1:
		printf("-Infinity%s", lint->xpathsep);
		break;
	    default:
		if (xmlXPathIsNaN(cur->floatval)) {
		    printf("NaN%s", lint->xpathsep);
		} else {
		    printf("%0g%s", cur->floatval, lint->xpathsep);
		}
	    }
	    break;
        case XPATH_STRING:
	    printf("%s%s", (const char *) cur->stringval, lint->xpathsep);
	    break;
        case XPATH_UNDEFINED:
	    fprintf(lint->errStream, "XPath Object is uninitialized\n");
            lint->progresult = XMLLINT_ERR_XPATH;
	    break;
	default:
	    fprintf(lint->errStream, "XPath object of unexpected type\n");
            lint->progresult = XMLLINT_ERR_XPATH;
	    break;
    }
}

static void
doXPathQuery(xmllintState *lint, xmlDocPtr doc, const char *query) {
    xmlXPathContextPtr ctxt = NULL;
    xmlXPathCompExprPtr comp = NULL;
    xmlXPathObjectPtr res = NULL;

    ctxt = xmlXPathNewContext(doc);
    if (ctxt == NULL) {
        lint->progresult = XMLLINT_ERR_MEM;
        goto error;
    }

    comp = xmlXPathCtxtCompile(ctxt, BAD_CAST query);
    if (comp == NULL) {
        fprintf(lint->errStream, "XPath compilation failure\n");
        lint->progresult = XMLLINT_ERR_XPATH;
        goto error;
    }

#ifdef LIBXML_DEBUG_ENABLED
    if (lint->appOptions & XML_LINT_DEBUG_ENABLED) {
        xmlXPathDebugDumpCompExpr(stdout, comp, 0);
        printf("\n");
    }
#endif

    ctxt->node = (xmlNodePtr) doc;
    res = xmlXPathCompiledEval(comp, ctxt);
    if (res == NULL) {
        fprintf(lint->errStream, "XPath evaluation failure\n");
        lint->progresult = XMLLINT_ERR_XPATH;
        goto error;
    }

    doXPathDump(lint, res);

error:
    xmlXPathFreeObject(res);
    xmlXPathFreeCompExpr(comp);
    xmlXPathFreeContext(ctxt);
}
#endif /* LIBXML_XPATH_ENABLED */

/************************************************************************
 *									*
 *			Tree Test processing				*
 *									*
 ************************************************************************/

static xmlDocPtr
parseFile(xmllintState *lint, const char *filename) {
    xmlDocPtr doc = NULL;

    if ((lint->appOptions & XML_LINT_GENERATE) && (filename == NULL)) {
        xmlNodePtr n;

        doc = xmlNewDoc(BAD_CAST "1.0");
        if (doc == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            return(NULL);
        }
        n = xmlNewDocNode(doc, NULL, BAD_CAST "info", NULL);
        if (n == NULL) {
            xmlFreeDoc(doc);
            lint->progresult = XMLLINT_ERR_MEM;
            return(NULL);
        }
        if (xmlNodeSetContent(n, BAD_CAST "abc") < 0) {
            xmlFreeNode(n);
            xmlFreeDoc(doc);
            lint->progresult = XMLLINT_ERR_MEM;
            return(NULL);
        }
        xmlDocSetRootElement(doc, n);

        return(doc);
    }

#ifdef LIBXML_HTML_ENABLED
    if (lint->appOptions & XML_LINT_HTML_ENABLED) {
        doc = parseHtml(lint, filename);
        return(doc);
    }
#endif /* LIBXML_HTML_ENABLED */
    {
        doc = parseXml(lint, filename);
    }

    if (doc == NULL) {
        if (lint->ctxt->errNo == XML_ERR_NO_MEMORY)
            lint->progresult = XMLLINT_ERR_MEM;
        else
	    lint->progresult = XMLLINT_ERR_RDFILE;
    } else {
        xmlParserStatus status = xmlCtxtGetStatus(lint->ctxt);
        if ((lint->parseOptions & XML_PARSE_DTDVALID) &&
            (status & XML_STATUS_DTD_VALIDATION_FAILED))
            lint->progresult = XMLLINT_ERR_VALID;

        if ((lint->appOptions & XML_LINT_STRICT_NAMESPACE) &&
            (status & XML_STATUS_NOT_NS_WELL_FORMED)) {
            lint->progresult = XMLLINT_ERR_RDFILE;
        }
    }

    return(doc);
}

static void
parseAndPrintFile(xmllintState *lint, const char *filename) {
    FILE *errStream = lint->errStream;
    xmlDocPtr doc;

    /* Avoid unused variable warning */
    (void) errStream;

    if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1))
	startTimer(lint);

    doc = parseFile(lint, filename);
    if (doc == NULL) {
        if (lint->progresult == XMLLINT_RETURN_OK)
            lint->progresult = XMLLINT_ERR_UNCLASS;
	return;
    }

    if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	endTimer(lint, "Parsing");
    }

    if (lint->appOptions & XML_LINT_DROP_DTD) {
	xmlDtdPtr dtd;

	dtd = xmlGetIntSubset(doc);
	if (dtd != NULL) {
	    xmlUnlinkNode((xmlNodePtr)dtd);
            doc->intSubset = dtd;
	}
    }

#ifdef LIBXML_XINCLUDE_ENABLED
    if (lint->appOptions & XML_LINT_XINCLUDE) {
        xmlXIncludeCtxt *xinc;
        int res;

	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    startTimer(lint);
	}

        xinc = xmlXIncludeNewContext(doc);
        if (xinc == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            goto done;
        }
        xmlXIncludeSetResourceLoader(xinc, xmllintResourceLoader, lint);
        xmlXIncludeSetFlags(xinc, lint->parseOptions);
        res = xmlXIncludeProcessNode(xinc, (xmlNode *) doc);
        xmlXIncludeFreeContext(xinc);
        if (res < 0) {
            /*
             * Return an error but continue to print the document
             * to match long-standing behavior.
             */
	    lint->progresult = XMLLINT_ERR_UNCLASS;
        }

	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    endTimer(lint, "Xinclude processing");
	}
    }
#endif

    /*
     * shell interaction
     */
    if (lint->appOptions & XML_LINT_NAVIGATING_SHELL) {
#ifdef LIBXML_XPATH_ENABLED
        xmlXPathOrderDocElems(doc);
#endif
        xmllintShell(doc, filename, stdout);
        goto done;
    }

#ifdef LIBXML_XPATH_ENABLED
    if (lint->xpathquery != NULL) {
	xmlXPathOrderDocElems(doc);
        doXPathQuery(lint, doc, lint->xpathquery);
    }
#endif

    /*
     * test intermediate copy if needed.
     */
    if (lint->appOptions & XML_LINT_COPY_ENABLED) {
        xmlDocPtr tmp;

        tmp = doc;
	if (lint->appOptions & XML_LINT_TIMINGS) {
	    startTimer(lint);
	}
	doc = xmlCopyDoc(doc, 1);
        if (doc == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            xmlFreeDoc(tmp);
            return;
        }
	if (lint->appOptions & XML_LINT_TIMINGS) {
	    endTimer(lint, "Copying");
	}
	if (lint->appOptions & XML_LINT_TIMINGS) {
	    startTimer(lint);
	}
	xmlFreeDoc(tmp);
	if (lint->appOptions & XML_LINT_TIMINGS) {
	    endTimer(lint, "Freeing original");
	}
    }

#ifdef LIBXML_VALID_ENABLED
    if ((lint->appOptions & XML_LINT_VALID_INSERTIONS)
#ifdef LIBXML_HTML_ENABLED
        && ((lint->appOptions & XML_LINT_HTML_ENABLED) != XML_LINT_HTML_ENABLED)
#endif
    ) {
        const xmlChar* list[256];
	int nb, i;
	xmlNodePtr node;

	if (doc->children != NULL) {
	    node = doc->children;
	    while ((node != NULL) &&
                   ((node->type != XML_ELEMENT_NODE) ||
                    (node->last == NULL)))
                node = node->next;
	    if (node != NULL) {
		nb = xmlValidGetValidElements(node->last, NULL, list, 256);
		if (nb < 0) {
		    fprintf(errStream, "could not get valid list of elements\n");
		} else if (nb == 0) {
		    fprintf(errStream, "No element can be inserted under root\n");
		} else {
		    fprintf(errStream, "%d element types can be inserted under root:\n",
		           nb);
		    for (i = 0;i < nb;i++) {
			 fprintf(errStream, "%s\n", (char *) list[i]);
		    }
		}
	    }
	}
    } else
#endif /* LIBXML_VALID_ENABLED */
#ifdef LIBXML_READER_ENABLED
    if (lint->appOptions & XML_LINT_USE_WALKER) {
        walkDoc(lint, doc);
    }
#endif /* LIBXML_READER_ENABLED */
#ifdef LIBXML_OUTPUT_ENABLED
    if (lint->noout == 0) {
	/*
	 * print it.
	 */
#ifdef LIBXML_DEBUG_ENABLED
	if ((lint->appOptions & XML_LINT_DEBUG_ENABLED) != XML_LINT_DEBUG_ENABLED) {
#endif
	    if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
		startTimer(lint);
	    }
#ifdef LIBXML_C14N_ENABLED
            if (lint->appOptions & XML_LINT_CANONICAL_V1_0) {
	        xmlChar *result = NULL;
		int size;

		size = xmlC14NDocDumpMemory(doc, NULL, XML_C14N_1_0, NULL, 1, &result);
		if (size >= 0) {
		    if (write(1, result, size) == -1) {
		        fprintf(errStream, "Can't write data\n");
		    }
		    xmlFree(result);
		} else {
		    fprintf(errStream, "Failed to canonicalize\n");
		    lint->progresult = XMLLINT_ERR_OUT;
		}
	    } else if (lint->appOptions & XML_LINT_CANONICAL_V1_1) {
	        xmlChar *result = NULL;
		int size;

		size = xmlC14NDocDumpMemory(doc, NULL, XML_C14N_1_1, NULL, 1, &result);
		if (size >= 0) {
		    if (write(1, result, size) == -1) {
		        fprintf(errStream, "Can't write data\n");
		    }
		    xmlFree(result);
		} else {
		    fprintf(errStream, "Failed to canonicalize\n");
		    lint->progresult = XMLLINT_ERR_OUT;
		}
	    } else if (lint->appOptions & XML_LINT_CANONICAL_EXE) {
	        xmlChar *result = NULL;
		int size;

		size = xmlC14NDocDumpMemory(doc, NULL, XML_C14N_EXCLUSIVE_1_0, NULL, 1, &result);
		if (size >= 0) {
		    if (write(1, result, size) == -1) {
		        fprintf(errStream, "Can't write data\n");
		    }
		    xmlFree(result);
		} else {
		    fprintf(errStream, "Failed to canonicalize\n");
		    lint->progresult = XMLLINT_ERR_OUT;
		}
	    } else
#endif
            {
	        xmlSaveCtxtPtr ctxt = NULL;
		int saveOpts = 0;

                if (lint->format == 1)
		    saveOpts |= XML_SAVE_FORMAT;
                else if (lint->format == 2)
                    saveOpts |= XML_SAVE_WSNONSIG;

#if defined(LIBXML_HTML_ENABLED)
                if (lint->appOptions & XML_LINT_XML_OUT)
                    saveOpts |= XML_SAVE_AS_XML;
#endif

#ifdef LIBXML_ZLIB_ENABLED
	        if (lint->appOptions & XML_LINT_ZLIB_COMPRESSION) {
                    gzFile gz;

                    if (lint->output == NULL)
                        gz = gzdopen(STDOUT_FILENO, "wb9");
                    else
                        gz = gzopen(lint->output, "wb9");

                    if (gz != NULL)
                        ctxt = xmlSaveToIO(xmllintGzWrite, xmllintGzClose, gz,
                                           lint->encoding, saveOpts);
                } else
#endif
                {
                    if (lint->output == NULL)
                        ctxt = xmlSaveToFd(STDOUT_FILENO, lint->encoding,
                                           saveOpts);
                    else
                        ctxt = xmlSaveToFilename(lint->output, lint->encoding,
                                                 saveOpts);
                }

		if (ctxt != NULL) {
                    if (lint->indentString != NULL)
                        xmlSaveSetIndentString(ctxt, lint->indentString);

		    if (xmlSaveDoc(ctxt, doc) < 0) {
			fprintf(errStream, "failed save to %s\n",
				lint->output ? lint->output : "-");
			lint->progresult = XMLLINT_ERR_OUT;
		    }
		    xmlSaveClose(ctxt);
		} else {
                    fprintf(errStream, "failed save to %s\n",
                            lint->output ? lint->output : "-");
		    lint->progresult = XMLLINT_ERR_OUT;
		}
	    }
	    if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
		endTimer(lint, "Saving");
	    }
#ifdef LIBXML_DEBUG_ENABLED
	} else {
	    FILE *out;
	    if (lint->output == NULL)
	        out = stdout;
	    else {
		out = fopen(lint->output, "wb");
	    }
	    if (out != NULL) {
		xmlDebugDumpDocument(out, doc);

		if (lint->output != NULL)
		    fclose(out);
	    } else {
		fprintf(errStream, "failed to open %s\n", lint->output);
		lint->progresult = XMLLINT_ERR_OUT;
	    }
	}
#endif
    }
#endif /* LIBXML_OUTPUT_ENABLED */

#ifdef LIBXML_VALID_ENABLED
    /*
     * A posteriori validation test
     */
    if ((lint->dtdvalid != NULL) || (lint->dtdvalidfpi != NULL)) {
	xmlDtdPtr dtd;

	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    startTimer(lint);
	}
	if (lint->dtdvalid != NULL)
	    dtd = xmlParseDTD(NULL, BAD_CAST lint->dtdvalid);
	else
	    dtd = xmlParseDTD(BAD_CAST lint->dtdvalidfpi, NULL);
	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    endTimer(lint, "Parsing DTD");
	}
	if (dtd == NULL) {
	    if (lint->dtdvalid != NULL)
		fprintf(errStream, "Could not parse DTD %s\n",
                        lint->dtdvalid);
	    else
		fprintf(errStream, "Could not parse DTD %s\n",
                        lint->dtdvalidfpi);
	    lint->progresult = XMLLINT_ERR_DTD;
	} else {
	    xmlValidCtxtPtr cvp;

	    cvp = xmlNewValidCtxt();
	    if (cvp == NULL) {
                lint->progresult = XMLLINT_ERR_MEM;
                xmlFreeDtd(dtd);
                return;
	    }

	    if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
		startTimer(lint);
	    }
	    if (!xmlValidateDtd(cvp, doc, dtd)) {
		if (lint->dtdvalid != NULL)
		    fprintf(errStream,
			    "Document %s does not validate against %s\n",
			    filename, lint->dtdvalid);
		else
		    fprintf(errStream,
			    "Document %s does not validate against %s\n",
			    filename, lint->dtdvalidfpi);
		lint->progresult = XMLLINT_ERR_VALID;
	    }
	    if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
		endTimer(lint, "Validating against DTD");
	    }
	    xmlFreeValidCtxt(cvp);
	    xmlFreeDtd(dtd);
	}
    } else if (lint->appOptions & XML_LINT_POST_VALIDATION) {
	xmlValidCtxtPtr cvp;

	cvp = xmlNewValidCtxt();
	if (cvp == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            xmlFreeDoc(doc);
            return;
	}

	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    startTimer(lint);
	}
	if (!xmlValidateDocument(cvp, doc)) {
	    fprintf(errStream,
		    "Document %s does not validate\n", filename);
	    lint->progresult = XMLLINT_ERR_VALID;
	}
	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    endTimer(lint, "Validating");
	}
	xmlFreeValidCtxt(cvp);
    }
#endif /* LIBXML_VALID_ENABLED */
#ifdef LIBXML_SCHEMATRON_ENABLED
    if (lint->wxschematron != NULL) {
	xmlSchematronValidCtxtPtr ctxt;
	int ret;
	int flag;

	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    startTimer(lint);
	}

	if (lint->appOptions & XML_LINT_DEBUG_ENABLED)
	    flag = XML_SCHEMATRON_OUT_XML;
	else
	    flag = XML_SCHEMATRON_OUT_TEXT;
	if (lint->noout)
	    flag |= XML_SCHEMATRON_OUT_QUIET;
	ctxt = xmlSchematronNewValidCtxt(lint->wxschematron, flag);
        if (ctxt == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            xmlFreeDoc(doc);
            return;
        }
	ret = xmlSchematronValidateDoc(ctxt, doc);
	if (ret == 0) {
	    if ((lint->appOptions & XML_LINT_QUIET) != XML_LINT_QUIET) {
	        fprintf(errStream, "%s validates\n", filename);
	    }
	} else if (ret > 0) {
	    fprintf(errStream, "%s fails to validate\n", filename);
	    lint->progresult = XMLLINT_ERR_VALID;
	} else {
	    fprintf(errStream, "%s validation generated an internal error\n",
		   filename);
	    lint->progresult = XMLLINT_ERR_VALID;
	}
	xmlSchematronFreeValidCtxt(ctxt);
	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    endTimer(lint, "Validating");
	}
    }
#endif

#ifdef LIBXML_RELAXNG_ENABLED
    if (lint->relaxngschemas != NULL) {
	xmlRelaxNGValidCtxtPtr ctxt;
	int ret;

	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    startTimer(lint);
	}

	ctxt = xmlRelaxNGNewValidCtxt(lint->relaxngschemas);
        if (ctxt == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            xmlFreeDoc(doc);
            return;
        }
	ret = xmlRelaxNGValidateDoc(ctxt, doc);
	if (ret == 0) {
	    if ((lint->appOptions & XML_LINT_QUIET) != XML_LINT_QUIET) {
	        fprintf(errStream, "%s validates\n", filename);
	    }
	} else if (ret > 0) {
	    fprintf(errStream, "%s fails to validate\n", filename);
	    lint->progresult = XMLLINT_ERR_VALID;
	} else {
	    fprintf(errStream, "%s validation generated an internal error\n",
		   filename);
	    lint->progresult = XMLLINT_ERR_VALID;
	}
	xmlRelaxNGFreeValidCtxt(ctxt);
	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    endTimer(lint, "Validating");
	}
    }
#endif /* LIBXML_RELAXNG_ENABLED */

#ifdef LIBXML_SCHEMAS_ENABLED
    if (lint->wxschemas != NULL) {
	xmlSchemaValidCtxtPtr ctxt;
	int ret;

	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    startTimer(lint);
	}

	ctxt = xmlSchemaNewValidCtxt(lint->wxschemas);
        if (ctxt == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            xmlFreeDoc(doc);
            return;
        }
	ret = xmlSchemaValidateDoc(ctxt, doc);
	if (ret == 0) {
	    if ((lint->appOptions & XML_LINT_QUIET) != XML_LINT_QUIET) {
	        fprintf(errStream, "%s validates\n", filename);
	    }
	} else if (ret > 0) {
	    fprintf(errStream, "%s fails to validate\n", filename);
	    lint->progresult = XMLLINT_ERR_VALID;
	} else {
	    fprintf(errStream, "%s validation generated an internal error\n",
		   filename);
	    lint->progresult = XMLLINT_ERR_VALID;
	}
	xmlSchemaFreeValidCtxt(ctxt);
	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	    endTimer(lint, "Validating");
	}
    }
#endif /* LIBXML_SCHEMAS_ENABLED */

    /* Avoid unused label warning */
    goto done;

done:
    /*
     * free it.
     */
    if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	startTimer(lint);
    }
    xmlFreeDoc(doc);
    if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat == 1)) {
	endTimer(lint, "Freeing");
    }
}

/************************************************************************
 *									*
 *			Usage and Main					*
 *									*
 ************************************************************************/

static void showVersion(FILE *errStream, const char *name) {
    fprintf(errStream, "%s: using libxml version %s\n", name, xmlParserVersion);
    fprintf(errStream, "   compiled with: ");
    if (xmlHasFeature(XML_WITH_THREAD)) fprintf(errStream, "Threads ");
    fprintf(errStream, "Tree ");
    if (xmlHasFeature(XML_WITH_OUTPUT)) fprintf(errStream, "Output ");
    if (xmlHasFeature(XML_WITH_PUSH)) fprintf(errStream, "Push ");
    if (xmlHasFeature(XML_WITH_READER)) fprintf(errStream, "Reader ");
    if (xmlHasFeature(XML_WITH_PATTERN)) fprintf(errStream, "Patterns ");
    if (xmlHasFeature(XML_WITH_WRITER)) fprintf(errStream, "Writer ");
    if (xmlHasFeature(XML_WITH_SAX1)) fprintf(errStream, "SAXv1 ");
    if (xmlHasFeature(XML_WITH_VALID)) fprintf(errStream, "DTDValid ");
    if (xmlHasFeature(XML_WITH_HTML)) fprintf(errStream, "HTML ");
    if (xmlHasFeature(XML_WITH_C14N)) fprintf(errStream, "C14N ");
    if (xmlHasFeature(XML_WITH_CATALOG)) fprintf(errStream, "Catalog ");
    if (xmlHasFeature(XML_WITH_XPATH)) fprintf(errStream, "XPath ");
    if (xmlHasFeature(XML_WITH_XPTR)) fprintf(errStream, "XPointer ");
    if (xmlHasFeature(XML_WITH_XINCLUDE)) fprintf(errStream, "XInclude ");
    if (xmlHasFeature(XML_WITH_ICONV)) fprintf(errStream, "Iconv ");
    if (xmlHasFeature(XML_WITH_ICU)) fprintf(errStream, "ICU ");
    if (xmlHasFeature(XML_WITH_ISO8859X)) fprintf(errStream, "ISO8859X ");
    if (xmlHasFeature(XML_WITH_REGEXP))
        fprintf(errStream, "Regexps Automata ");
    if (xmlHasFeature(XML_WITH_RELAXNG)) fprintf(errStream, "RelaxNG ");
    if (xmlHasFeature(XML_WITH_SCHEMAS)) fprintf(errStream, "Schemas ");
    if (xmlHasFeature(XML_WITH_SCHEMATRON)) fprintf(errStream, "Schematron ");
    if (xmlHasFeature(XML_WITH_MODULES)) fprintf(errStream, "Modules ");
    if (xmlHasFeature(XML_WITH_DEBUG)) fprintf(errStream, "Debug ");
    if (xmlHasFeature(XML_WITH_ZLIB)) fprintf(errStream, "Zlib ");
    fprintf(errStream, "\n");
}

static void usage(FILE *f, const char *name) {
    fprintf(f, "Usage : %s [options] XMLfiles ...\n", name);
#ifdef LIBXML_OUTPUT_ENABLED
    fprintf(f, "\tParse the XML files and output the result of the parsing\n");
#else
    fprintf(f, "\tParse the XML files\n");
#endif /* LIBXML_OUTPUT_ENABLED */
    fprintf(f, "\t--version : display the version of the XML library used\n");
    fprintf(f, "\t--shell : run a navigating shell\n");
    fprintf(f, "\t--debug : show additional debug information\n");
    fprintf(f, "\t--copy : used to test the internal copy implementation\n");
    fprintf(f, "\t--recover : output what was parsable on broken XML documents\n");
    fprintf(f, "\t--huge : remove any internal arbitrary parser limits\n");
    fprintf(f, "\t--noent : substitute entity references by their value\n");
    fprintf(f, "\t--no-xxe : disable loading of external entities\n");
    fprintf(f, "\t--noenc : ignore any encoding specified inside the document\n");
    fprintf(f, "\t--noout : don't output the result tree\n");
    fprintf(f, "\t--path 'paths': provide a set of paths for resources\n");
    fprintf(f, "\t--load-trace : print trace of all external entities loaded\n");
    fprintf(f, "\t--nonet : refuse to fetch DTDs or entities over network\n");
    fprintf(f, "\t--nocompact : do not generate compact text nodes\n");
#ifdef LIBXML_VALID_ENABLED
    fprintf(f, "\t--valid : validate the document in addition to std well-formed check\n");
    fprintf(f, "\t--postvalid : do a posteriori validation, i.e after parsing\n");
    fprintf(f, "\t--dtdvalid URL : do a posteriori validation against a given DTD\n");
    fprintf(f, "\t--dtdvalidfpi FPI : same but name the DTD with a Public Identifier\n");
    fprintf(f, "\t--insert : ad-hoc test for valid insertions\n");
#endif /* LIBXML_VALID_ENABLED */
    fprintf(f, "\t--strict-namespace : Return application failure if document has any namespace errors\n");
    fprintf(f, "\t--quiet : be quiet when succeeded\n");
    fprintf(f, "\t--timing : print some timings\n");
    fprintf(f, "\t--repeat : repeat 100 times, for timing or profiling\n");
    fprintf(f, "\t--dropdtd : remove the DOCTYPE of the input docs\n");
#ifdef LIBXML_HTML_ENABLED
    fprintf(f, "\t--html : use the HTML parser\n");
    fprintf(f, "\t--nodefdtd : do not default HTML doctype\n");
#ifdef LIBXML_OUTPUT_ENABLED
    fprintf(f, "\t--xmlout : force to use the XML serializer when using --html\n");
#endif
#endif
#ifdef LIBXML_PUSH_ENABLED
    fprintf(f, "\t--push : use the push mode of the parser\n");
#endif /* LIBXML_PUSH_ENABLED */
#if HAVE_DECL_MMAP
    fprintf(f, "\t--memory : parse from memory\n");
#endif
    fprintf(f, "\t--maxmem nbbytes : limits memory allocation to nbbytes bytes\n");
    fprintf(f, "\t--nowarning : do not emit warnings from parser/validator\n");
    fprintf(f, "\t--error-ring N : keep last N errors in a ring buffer\n");
    fprintf(f, "\t--error-dedup N : suppress repeated errors after N occurrences\n");
    fprintf(f, "\t--error-ring-dump : dump error ring as JSON after each file\n");
    fprintf(f, "\t--error-ring-dump-file FILE : dump error ring JSON to FILE\n");
    fprintf(f, "\t--error-ring-dump-cbor-file FILE : dump error ring as CBOR to FILE\n");
    fprintf(f, "\t--error-ring-dump-bin-file FILE : dump error ring as binary frames to FILE\n");
    fprintf(f, "\t--error-xml : emit structured errors as XML lines\n");
    fprintf(f, "\t--error-xml-file FILE : write XML errors to FILE\n");
    fprintf(f, "\t--error-json : emit structured errors as JSON lines\n");
    fprintf(f, "\t--error-json-file FILE : write JSON errors to FILE\n");
    fprintf(f, "\t--error-json-warn-file FILE : write JSON warnings to FILE\n");
    fprintf(f, "\t--error-json-array : emit a JSON array per file\n");
    fprintf(f, "\t--error-json-pretty : pretty-print JSON output\n");
    fprintf(f, "\t--error-json-limit N : limit JSON errors emitted\n");
    fprintf(f, "\t--error-json-summary : emit per-file summary JSON\n");
    fprintf(f, "\t--error-json-window N : include N bytes of input context\n");
    fprintf(f, "\t--error-json-checksum : include file checksum\n");
    fprintf(f, "\t--error-redact LIST : redact fields (file,message,str1,str2,str3,window,all,none)\n");
    fprintf(f, "\t--error-syslog : send JSON errors to syslog\n");
    fprintf(f, "\t--error-syslog-facility NAME : set syslog facility (user,local0..local7)\n");
    fprintf(f, "\t--noblanks : drop (ignorable?) blanks spaces\n");
    fprintf(f, "\t--nocdata : replace cdata section with text nodes\n");
    fprintf(f, "\t--nodict : create document without dictionary\n");
    fprintf(f, "\t--pedantic : enable additional warnings\n");
#ifdef LIBXML_OUTPUT_ENABLED
    fprintf(f, "\t--output file or -o file: save to a given file\n");
    fprintf(f, "\t--format : reformat/reindent the output\n");
    fprintf(f, "\t--encode encoding : output in the given encoding\n");
    fprintf(f, "\t--pretty STYLE : pretty-print in a particular style\n");
    fprintf(f, "\t                 0 Do not pretty print\n");
    fprintf(f, "\t                 1 Format the XML content, as --format\n");
    fprintf(f, "\t                 2 Add whitespace inside tags, preserving content\n");
#ifdef LIBXML_ZLIB_ENABLED
    fprintf(f, "\t--compress : turn on gzip compression of output\n");
#endif
#ifdef LIBXML_C14N_ENABLED
    fprintf(f, "\t--c14n : save in W3C canonical format v1.0 (with comments)\n");
    fprintf(f, "\t--c14n11 : save in W3C canonical format v1.1 (with comments)\n");
    fprintf(f, "\t--exc-c14n : save in W3C exclusive canonical format (with comments)\n");
#endif /* LIBXML_C14N_ENABLED */
#endif /* LIBXML_OUTPUT_ENABLED */
    fprintf(f, "\t--nsclean : remove redundant namespace declarations\n");
#ifdef LIBXML_CATALOG_ENABLED
    fprintf(f, "\t--catalogs : use SGML catalogs from $SGML_CATALOG_FILES\n");
    fprintf(f, "\t             otherwise XML Catalogs starting from \n");
    fprintf(f, "\t         file://" XML_SYSCONFDIR "/xml/catalog "
            "are activated by default\n");
    fprintf(f, "\t--nocatalogs: deactivate all catalogs\n");
#endif
    fprintf(f, "\t--auto : generate a small doc on the fly\n");
#ifdef LIBXML_XINCLUDE_ENABLED
    fprintf(f, "\t--xinclude : do XInclude processing\n");
    fprintf(f, "\t--noxincludenode : same but do not generate XInclude nodes\n");
    fprintf(f, "\t--nofixup-base-uris : do not fixup xml:base uris\n");
#endif
    fprintf(f, "\t--loaddtd : fetch external DTD\n");
    fprintf(f, "\t--dtdattr : loaddtd + populate the tree with inherited attributes \n");
#ifdef LIBXML_READER_ENABLED
    fprintf(f, "\t--stream : use the streaming interface to process very large files\n");
    fprintf(f, "\t--walker : create a reader and walk though the resulting doc\n");
#ifdef LIBXML_PATTERN_ENABLED
    fprintf(f, "\t--pattern pattern_value : test the pattern support\n");
#endif
#endif /* LIBXML_READER_ENABLED */
#ifdef LIBXML_RELAXNG_ENABLED
    fprintf(f, "\t--relaxng schema : do RelaxNG validation against the schema\n");
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
    fprintf(f, "\t--schema schema : do validation against the WXS schema\n");
#endif
#ifdef LIBXML_SCHEMATRON_ENABLED
    fprintf(f, "\t--schematron schema : do validation against a schematron\n");
#endif
#ifdef LIBXML_SAX1_ENABLED
    fprintf(f, "\t--sax1: use the old SAX1 interfaces for processing\n");
#endif
    fprintf(f, "\t--sax: do not build a tree but work just at the SAX level\n");
    fprintf(f, "\t--oldxml10: use XML-1.0 parsing rules before the 5th edition\n");
#ifdef LIBXML_XPATH_ENABLED
    fprintf(f, "\t--xpath expr: evaluate the XPath expression, results are separated by \\n, imply --noout\n");
    fprintf(f, "\t--xpath0 expr: evaluate the XPath expression, results are separated by \\0, imply --noout\n");
#endif
    fprintf(f, "\t--max-ampl value: set maximum amplification factor\n");

    fprintf(f, "\nLibxml project home page: https://gitlab.gnome.org/GNOME/libxml2\n");
}

static int
parseInteger(unsigned long *result, FILE *errStream, const char *ctxt,
             const char *str, unsigned long min, unsigned long max) {
    char *strEnd;
    unsigned long val;

    errno = 0;
    val = strtoul(str, &strEnd, 10);
    if (errno == EINVAL || *strEnd != 0) {
        fprintf(errStream, "%s: invalid integer: %s\n", ctxt, str);
        return(-1);
    }
    if (errno != 0 || val < min || val > max) {
        fprintf(errStream, "%s: integer out of range: %s\n", ctxt, str);
        return(-1);
    }

    *result = val;
    return(0);
}

static int
xmllintParseRedact(xmllintState *lint, FILE *errStream, const char *value) {
    char *copy;
    char *token;
    unsigned flags;

    if ((lint == NULL) || (value == NULL))
        return(-1);

    copy = xmlMemStrdup(value);
    if (copy == NULL)
        return(-1);

    flags = lint->errorRedactFlags;
    for (token = strtok(copy, ",");
         token != NULL;
         token = strtok(NULL, ",")) {
        if (strcmp(token, "all") == 0) {
            flags = XMLLINT_REDACT_ALL;
        } else if (strcmp(token, "none") == 0) {
            flags = 0;
        } else if (strcmp(token, "file") == 0) {
            flags |= XMLLINT_REDACT_FILE;
        } else if (strcmp(token, "message") == 0) {
            flags |= XMLLINT_REDACT_MESSAGE;
        } else if (strcmp(token, "str1") == 0) {
            flags |= XMLLINT_REDACT_STR1;
        } else if (strcmp(token, "str2") == 0) {
            flags |= XMLLINT_REDACT_STR2;
        } else if (strcmp(token, "str3") == 0) {
            flags |= XMLLINT_REDACT_STR3;
        } else if (strcmp(token, "window") == 0) {
            flags |= XMLLINT_REDACT_WINDOW;
        } else {
            fprintf(errStream, "error-redact: unknown token '%s'\n", token);
            xmlFree(copy);
            return(-1);
        }
    }

    lint->errorRedactFlags = flags;
    xmlFree(copy);
    return(0);
}

static int
skipArgs(const char *arg) {
    if ((!strcmp(arg, "-path")) ||
        (!strcmp(arg, "--path")) ||
        (!strcmp(arg, "-maxmem")) ||
        (!strcmp(arg, "--maxmem")) ||
        (!strcmp(arg, "-error-ring")) ||
        (!strcmp(arg, "--error-ring")) ||
#ifdef LIBXML_OUTPUT_ENABLED
        (!strcmp(arg, "-o")) ||
        (!strcmp(arg, "-output")) ||
        (!strcmp(arg, "--output")) ||
        (!strcmp(arg, "-encode")) ||
        (!strcmp(arg, "--encode")) ||
        (!strcmp(arg, "-pretty")) ||
        (!strcmp(arg, "--pretty")) ||
#endif
#ifdef LIBXML_VALID_ENABLED
        (!strcmp(arg, "-dtdvalid")) ||
        (!strcmp(arg, "--dtdvalid")) ||
        (!strcmp(arg, "-dtdvalidfpi")) ||
        (!strcmp(arg, "--dtdvalidfpi")) ||
#endif
#ifdef LIBXML_RELAXNG_ENABLED
        (!strcmp(arg, "-relaxng")) ||
        (!strcmp(arg, "--relaxng")) ||
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
        (!strcmp(arg, "-schema")) ||
        (!strcmp(arg, "--schema")) ||
#endif
#ifdef LIBXML_SCHEMATRON_ENABLED
        (!strcmp(arg, "-schematron")) ||
        (!strcmp(arg, "--schematron")) ||
#endif
#if defined(LIBXML_READER_ENABLED) && defined(LIBXML_PATTERN_ENABLED)
        (!strcmp(arg, "-pattern")) ||
        (!strcmp(arg, "--pattern")) ||
#endif
#ifdef LIBXML_XPATH_ENABLED
        (!strcmp(arg, "-xpath")) ||
        (!strcmp(arg, "--xpath")) ||
        (!strcmp(arg, "-xpath0")) ||
        (!strcmp(arg, "--xpath0")) ||
#endif
        (!strcmp(arg, "-error-ring")) ||
        (!strcmp(arg, "--error-ring")) ||
        (!strcmp(arg, "-error-ring-dump-file")) ||
        (!strcmp(arg, "--error-ring-dump-file")) ||
        (!strcmp(arg, "-error-ring-dump-cbor-file")) ||
        (!strcmp(arg, "--error-ring-dump-cbor-file")) ||
        (!strcmp(arg, "-error-ring-dump-bin-file")) ||
        (!strcmp(arg, "--error-ring-dump-bin-file")) ||
        (!strcmp(arg, "-error-xml-file")) ||
        (!strcmp(arg, "--error-xml-file")) ||
        (!strcmp(arg, "-error-json-file")) ||
        (!strcmp(arg, "--error-json-file")) ||
        (!strcmp(arg, "-error-json-warn-file")) ||
        (!strcmp(arg, "--error-json-warn-file")) ||
        (!strcmp(arg, "-error-redact")) ||
        (!strcmp(arg, "--error-redact")) ||
        (!strcmp(arg, "-error-json-limit")) ||
        (!strcmp(arg, "--error-json-limit")) ||
        (!strcmp(arg, "-error-json-window")) ||
        (!strcmp(arg, "--error-json-window")) ||
        (!strcmp(arg, "-error-dedup")) ||
        (!strcmp(arg, "--error-dedup")) ||
        (!strcmp(arg, "-max-ampl")) ||
        (!strcmp(arg, "--max-ampl"))
    ) {
        return(1);
    }

    return(0);
}

static void
xmllintInit(xmllintState *lint) {
    memset(lint, 0, sizeof(*lint));

    lint->repeat = 1;
    lint->progresult = XMLLINT_RETURN_OK;
    lint->parseOptions = XML_PARSE_COMPACT | XML_PARSE_BIG_LINES;
    lint->errorRingSize = 0;
    lint->errorDedupLimit = 0;
    lint->errorRingDump = 0;
    lint->errorXml = 0;
    lint->errorJson = 0;
    lint->errorJsonArray = 0;
    lint->errorJsonPretty = 0;
    lint->errorJsonLimit = 0;
    lint->errorJsonCount = 0;
    lint->errorJsonArrayOpen = 0;
    lint->errorJsonArrayCount = 0;
    lint->errorJsonArrayFile = NULL;
    lint->errorJsonSummary = 0;
    lint->errorJsonWindow = 0;
    lint->errorJsonChecksum = 0;
    lint->errorSyslog = 0;
    lint->errorSyslogFacility = LOG_USER;
    lint->errorRedactFlags = 0;
    lint->errorRingDumpFile = NULL;
    lint->errorRingDumpCborFile = NULL;
    lint->errorRingDumpBinFile = NULL;
    lint->errorXmlFile = NULL;
    lint->errorJsonFile = NULL;
    lint->errorJsonWarnFile = NULL;
    lint->errorRingDumpStream = NULL;
    lint->errorRingDumpCborStream = NULL;
    lint->errorRingDumpBinStream = NULL;
    lint->errorXmlStream = NULL;
    lint->errorJsonStream = NULL;
    lint->errorJsonWarnStream = NULL;
    lint->errorChecksum = 0;
    lint->checksumValid = 0;
    lint->errorCodeCounts = NULL;
    lint->errorCodeCountsSize = 0;
    lint->errorCodeCountsCount = 0;
    lint->errorStageCounts = NULL;
    lint->errorStageCountsSize = 0;
    lint->errorStageCountsCount = 0;
#ifdef LIBXML_HTML_ENABLED
    lint->htmlOptions = HTML_PARSE_COMPACT | HTML_PARSE_BIG_LINES;
#endif
}

static void
xmllintOptWarnNoSupport(FILE *errStream, const char *opt, const char *nosupp) {
    fprintf(errStream, "Warning: Option %s doesn't support %s\n", opt, nosupp);
}

static int
xmllintParseOptions(xmllintState *lint, int argc, const char **argv) {
    FILE *errStream = lint->errStream;
    const char *specialMode = NULL;
    int i;

    if (argc <= 1) {
        usage(errStream, argv[0]);
        return(XMLLINT_ERR_UNCLASS);
    }

    for (i = 1; i < argc ; i++) {
        unsigned long val;

        if (argv[i][0] != '-' || argv[i][1] == 0)
            continue;

        if ((!strcmp(argv[i], "-maxmem")) ||
            (!strcmp(argv[i], "--maxmem"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "maxmem: missing integer value\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            if (parseInteger(&val, errStream, "maxmem", argv[i],
                             0, INT_MAX) < 0)
                return(XMLLINT_ERR_UNCLASS);
            lint->maxmem = val;
        } else if ((!strcmp(argv[i], "-debug")) ||
                   (!strcmp(argv[i], "--debug"))) {
            lint->appOptions |= XML_LINT_DEBUG_ENABLED;
        } else if ((!strcmp(argv[i], "-shell")) ||
                   (!strcmp(argv[i], "--shell"))) {
            lint->appOptions |= XML_LINT_NAVIGATING_SHELL;
        } else if ((!strcmp(argv[i], "-copy")) ||
                   (!strcmp(argv[i], "--copy"))) {
            lint->appOptions |= XML_LINT_COPY_ENABLED;
        } else if ((!strcmp(argv[i], "-recover")) ||
                   (!strcmp(argv[i], "--recover"))) {
            lint->parseOptions |= XML_PARSE_RECOVER;
        } else if ((!strcmp(argv[i], "-huge")) ||
                   (!strcmp(argv[i], "--huge"))) {
            lint->parseOptions |= XML_PARSE_HUGE;
#ifdef LIBXML_HTML_ENABLED
            lint->htmlOptions |= HTML_PARSE_HUGE;
#endif
        } else if ((!strcmp(argv[i], "-noent")) ||
                   (!strcmp(argv[i], "--noent"))) {
            lint->parseOptions |= XML_PARSE_NOENT;
        } else if ((!strcmp(argv[i], "-no-xxe")) ||
                   (!strcmp(argv[i], "--no-xxe"))) {
            lint->parseOptions |= XML_PARSE_NO_XXE;
        } else if ((!strcmp(argv[i], "-noenc")) ||
                   (!strcmp(argv[i], "--noenc"))) {
            lint->parseOptions |= XML_PARSE_IGNORE_ENC;
#ifdef LIBXML_HTML_ENABLED
            lint->htmlOptions |= HTML_PARSE_IGNORE_ENC;
#endif
        } else if ((!strcmp(argv[i], "-nsclean")) ||
                   (!strcmp(argv[i], "--nsclean"))) {
            lint->parseOptions |= XML_PARSE_NSCLEAN;
        } else if ((!strcmp(argv[i], "-nocdata")) ||
                   (!strcmp(argv[i], "--nocdata"))) {
            lint->parseOptions |= XML_PARSE_NOCDATA;
        } else if ((!strcmp(argv[i], "-nodict")) ||
                   (!strcmp(argv[i], "--nodict"))) {
            lint->parseOptions |= XML_PARSE_NODICT;
        } else if ((!strcmp(argv[i], "-version")) ||
                   (!strcmp(argv[i], "--version"))) {
            showVersion(errStream, argv[0]);
            lint->version = 1;
        } else if ((!strcmp(argv[i], "-noout")) ||
                   (!strcmp(argv[i], "--noout"))) {
            lint->noout = 1;
#ifdef LIBXML_HTML_ENABLED
        } else if ((!strcmp(argv[i], "-html")) ||
                   (!strcmp(argv[i], "--html"))) {
            lint->appOptions |= XML_LINT_HTML_ENABLED;
        } else if ((!strcmp(argv[i], "-nodefdtd")) ||
                   (!strcmp(argv[i], "--nodefdtd"))) {
            lint->htmlOptions |= HTML_PARSE_NODEFDTD;
#ifdef LIBXML_OUTPUT_ENABLED
        } else if ((!strcmp(argv[i], "-xmlout")) ||
                   (!strcmp(argv[i], "--xmlout"))) {
            lint->appOptions |= XML_LINT_XML_OUT;
#endif
#endif /* LIBXML_HTML_ENABLED */
        } else if ((!strcmp(argv[i], "-loaddtd")) ||
                   (!strcmp(argv[i], "--loaddtd"))) {
            lint->parseOptions |= XML_PARSE_DTDLOAD;
        } else if ((!strcmp(argv[i], "-dtdattr")) ||
                   (!strcmp(argv[i], "--dtdattr"))) {
            lint->parseOptions |= XML_PARSE_DTDATTR;
#ifdef LIBXML_VALID_ENABLED
        } else if ((!strcmp(argv[i], "-valid")) ||
                   (!strcmp(argv[i], "--valid"))) {
            lint->parseOptions |= XML_PARSE_DTDVALID;
        } else if ((!strcmp(argv[i], "-postvalid")) ||
                   (!strcmp(argv[i], "--postvalid"))) {
            lint->appOptions |= XML_LINT_POST_VALIDATION;
            lint->parseOptions |= XML_PARSE_DTDLOAD;
        } else if ((!strcmp(argv[i], "-dtdvalid")) ||
                   (!strcmp(argv[i], "--dtdvalid"))) {
            i++;
            lint->dtdvalid = argv[i];
            lint->parseOptions |= XML_PARSE_DTDLOAD;
        } else if ((!strcmp(argv[i], "-dtdvalidfpi")) ||
                   (!strcmp(argv[i], "--dtdvalidfpi"))) {
            i++;
            lint->dtdvalidfpi = argv[i];
            lint->parseOptions |= XML_PARSE_DTDLOAD;
        } else if ((!strcmp(argv[i], "-insert")) ||
                   (!strcmp(argv[i], "--insert"))) {
            lint->appOptions |= XML_LINT_VALID_INSERTIONS;
#endif /* LIBXML_VALID_ENABLED */
        } else if ((!strcmp(argv[i], "-strict-namespace")) ||
            (!strcmp(argv[i], "--strict-namespace"))) {
            lint->appOptions |= XML_LINT_STRICT_NAMESPACE;
        } else if ((!strcmp(argv[i], "-dropdtd")) ||
                   (!strcmp(argv[i], "--dropdtd"))) {
            lint->appOptions |= XML_LINT_DROP_DTD;
        } else if ((!strcmp(argv[i], "-quiet")) ||
                   (!strcmp(argv[i], "--quiet"))) {
            lint->appOptions |= XML_LINT_QUIET;
        } else if ((!strcmp(argv[i], "-timing")) ||
                   (!strcmp(argv[i], "--timing"))) {
            lint->appOptions |= XML_LINT_TIMINGS;
        } else if ((!strcmp(argv[i], "-auto")) ||
                   (!strcmp(argv[i], "--auto"))) {
            lint->appOptions |= XML_LINT_GENERATE;
        } else if ((!strcmp(argv[i], "-repeat")) ||
                   (!strcmp(argv[i], "--repeat"))) {
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
            lint->repeat = 2;
#else
            if (lint->repeat > 1)
                lint->repeat *= 10;
            else
                lint->repeat = 100;
#endif
#ifdef LIBXML_PUSH_ENABLED
        } else if ((!strcmp(argv[i], "-push")) ||
                   (!strcmp(argv[i], "--push"))) {
            lint->appOptions |= XML_LINT_PUSH_ENABLED;
#endif /* LIBXML_PUSH_ENABLED */
#if HAVE_DECL_MMAP
        } else if ((!strcmp(argv[i], "-memory")) ||
                   (!strcmp(argv[i], "--memory"))) {
            lint->appOptions |= XML_LINT_MEMORY;
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
        } else if ((!strcmp(argv[i], "-xinclude")) ||
                   (!strcmp(argv[i], "--xinclude"))) {
            lint->appOptions |= XML_LINT_XINCLUDE;
            lint->parseOptions |= XML_PARSE_XINCLUDE;
        } else if ((!strcmp(argv[i], "-noxincludenode")) ||
                   (!strcmp(argv[i], "--noxincludenode"))) {
            lint->appOptions |= XML_LINT_XINCLUDE;
            lint->parseOptions |= XML_PARSE_XINCLUDE;
            lint->parseOptions |= XML_PARSE_NOXINCNODE;
        } else if ((!strcmp(argv[i], "-nofixup-base-uris")) ||
                   (!strcmp(argv[i], "--nofixup-base-uris"))) {
            lint->appOptions |= XML_LINT_XINCLUDE;
            lint->parseOptions |= XML_PARSE_XINCLUDE;
            lint->parseOptions |= XML_PARSE_NOBASEFIX;
#endif
        } else if ((!strcmp(argv[i], "-nowarning")) ||
                   (!strcmp(argv[i], "--nowarning"))) {
            lint->parseOptions |= XML_PARSE_NOWARNING;
            lint->parseOptions &= ~XML_PARSE_PEDANTIC;
#ifdef LIBXML_HTML_ENABLED
            lint->htmlOptions |= HTML_PARSE_NOWARNING;
#endif
        } else if ((!strcmp(argv[i], "-error-ring")) ||
                   (!strcmp(argv[i], "--error-ring"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-ring: missing integer value\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            if (parseInteger(&val, errStream, "error-ring", argv[i],
                             0, INT_MAX) < 0)
                return(XMLLINT_ERR_UNCLASS);
            lint->errorRingSize = (int) val;
        } else if ((!strcmp(argv[i], "-error-dedup")) ||
                   (!strcmp(argv[i], "--error-dedup"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-dedup: missing integer value\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            if (parseInteger(&val, errStream, "error-dedup", argv[i],
                             0, INT_MAX) < 0)
                return(XMLLINT_ERR_UNCLASS);
            lint->errorDedupLimit = (int) val;
        } else if ((!strcmp(argv[i], "-error-ring-dump")) ||
                   (!strcmp(argv[i], "--error-ring-dump"))) {
            lint->errorRingDump = 1;
        } else if ((!strcmp(argv[i], "-error-ring-dump-file")) ||
                   (!strcmp(argv[i], "--error-ring-dump-file"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-ring-dump-file: missing filename\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            lint->errorRingDumpFile = argv[i];
        } else if ((!strcmp(argv[i], "-error-ring-dump-cbor-file")) ||
                   (!strcmp(argv[i], "--error-ring-dump-cbor-file"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-ring-dump-cbor-file: missing filename\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            lint->errorRingDumpCborFile = argv[i];
        } else if ((!strcmp(argv[i], "-error-ring-dump-bin-file")) ||
                   (!strcmp(argv[i], "--error-ring-dump-bin-file"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-ring-dump-bin-file: missing filename\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            lint->errorRingDumpBinFile = argv[i];
        } else if ((!strcmp(argv[i], "-error-xml")) ||
                   (!strcmp(argv[i], "--error-xml"))) {
            lint->errorXml = 1;
        } else if ((!strcmp(argv[i], "-error-xml-file")) ||
                   (!strcmp(argv[i], "--error-xml-file"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-xml-file: missing filename\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            lint->errorXmlFile = argv[i];
            lint->errorXml = 1;
        } else if ((!strcmp(argv[i], "-error-json")) ||
                   (!strcmp(argv[i], "--error-json"))) {
            lint->errorJson = 1;
        } else if ((!strcmp(argv[i], "-error-json-file")) ||
                   (!strcmp(argv[i], "--error-json-file"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-json-file: missing filename\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            lint->errorJsonFile = argv[i];
            lint->errorJson = 1;
        } else if ((!strcmp(argv[i], "-error-json-warn-file")) ||
                   (!strcmp(argv[i], "--error-json-warn-file"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-json-warn-file: missing filename\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            lint->errorJsonWarnFile = argv[i];
            lint->errorJson = 1;
        } else if ((!strcmp(argv[i], "-error-json-array")) ||
                   (!strcmp(argv[i], "--error-json-array"))) {
            lint->errorJsonArray = 1;
            lint->errorJson = 1;
        } else if ((!strcmp(argv[i], "-error-json-pretty")) ||
                   (!strcmp(argv[i], "--error-json-pretty"))) {
            lint->errorJsonPretty = 1;
            lint->errorJson = 1;
        } else if ((!strcmp(argv[i], "-error-json-summary")) ||
                   (!strcmp(argv[i], "--error-json-summary"))) {
            lint->errorJsonSummary = 1;
            lint->errorJson = 1;
        } else if ((!strcmp(argv[i], "-error-json-window")) ||
                   (!strcmp(argv[i], "--error-json-window"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-json-window: missing integer value\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            if (parseInteger(&val, errStream, "error-json-window", argv[i],
                             0, INT_MAX) < 0)
                return(XMLLINT_ERR_UNCLASS);
            lint->errorJsonWindow = (int) val;
            lint->errorJson = 1;
        } else if ((!strcmp(argv[i], "-error-redact")) ||
                   (!strcmp(argv[i], "--error-redact"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-redact: missing value\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            if (xmllintParseRedact(lint, errStream, argv[i]) < 0)
                return(XMLLINT_ERR_UNCLASS);
        } else if ((!strcmp(argv[i], "-error-json-checksum")) ||
                   (!strcmp(argv[i], "--error-json-checksum"))) {
            lint->errorJsonChecksum = 1;
            lint->errorJson = 1;
        } else if ((!strcmp(argv[i], "-error-syslog")) ||
                   (!strcmp(argv[i], "--error-syslog"))) {
#ifndef _WIN32
            lint->errorSyslog = 1;
            lint->errorJson = 1;
#else
            fprintf(errStream, "Warning: --error-syslog unsupported on Windows\n");
#endif
        } else if ((!strcmp(argv[i], "-error-syslog-facility")) ||
                   (!strcmp(argv[i], "--error-syslog-facility"))) {
#ifndef _WIN32
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-syslog-facility: missing value\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            lint->errorSyslog = 1;
            lint->errorJson = 1;
            if (!strcmp(argv[i], "auth"))
                lint->errorSyslogFacility = LOG_AUTH;
            else if (!strcmp(argv[i], "authpriv"))
                lint->errorSyslogFacility = LOG_AUTHPRIV;
            else if (!strcmp(argv[i], "daemon"))
                lint->errorSyslogFacility = LOG_DAEMON;
            else if (!strcmp(argv[i], "user"))
                lint->errorSyslogFacility = LOG_USER;
            else if (!strcmp(argv[i], "local0"))
                lint->errorSyslogFacility = LOG_LOCAL0;
            else if (!strcmp(argv[i], "local1"))
                lint->errorSyslogFacility = LOG_LOCAL1;
            else if (!strcmp(argv[i], "local2"))
                lint->errorSyslogFacility = LOG_LOCAL2;
            else if (!strcmp(argv[i], "local3"))
                lint->errorSyslogFacility = LOG_LOCAL3;
            else if (!strcmp(argv[i], "local4"))
                lint->errorSyslogFacility = LOG_LOCAL4;
            else if (!strcmp(argv[i], "local5"))
                lint->errorSyslogFacility = LOG_LOCAL5;
            else if (!strcmp(argv[i], "local6"))
                lint->errorSyslogFacility = LOG_LOCAL6;
            else if (!strcmp(argv[i], "local7"))
                lint->errorSyslogFacility = LOG_LOCAL7;
            else {
                fprintf(errStream, "error-syslog-facility: invalid value %s\n",
                        argv[i]);
                return(XMLLINT_ERR_UNCLASS);
            }
#else
            fprintf(errStream, "Warning: --error-syslog-facility unsupported on Windows\n");
#endif
        } else if ((!strcmp(argv[i], "-error-json-limit")) ||
                   (!strcmp(argv[i], "--error-json-limit"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "error-json-limit: missing integer value\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            if (parseInteger(&val, errStream, "error-json-limit", argv[i],
                             0, INT_MAX) < 0)
                return(XMLLINT_ERR_UNCLASS);
            lint->errorJsonLimit = (int) val;
            lint->errorJson = 1;
        } else if ((!strcmp(argv[i], "-pedantic")) ||
                   (!strcmp(argv[i], "--pedantic"))) {
            lint->parseOptions |= XML_PARSE_PEDANTIC;
            lint->parseOptions &= ~XML_PARSE_NOWARNING;
#ifdef LIBXML_CATALOG_ENABLED
        } else if ((!strcmp(argv[i], "-catalogs")) ||
                   (!strcmp(argv[i], "--catalogs"))) {
            lint->appOptions |= XML_LINT_USE_CATALOGS;
        } else if ((!strcmp(argv[i], "-nocatalogs")) ||
                   (!strcmp(argv[i], "--nocatalogs"))) {
            lint->appOptions |= XML_LINT_USE_NO_CATALOGS;
            lint->parseOptions |= XML_PARSE_NO_SYS_CATALOG;
#endif
        } else if ((!strcmp(argv[i], "-noblanks")) ||
                   (!strcmp(argv[i], "--noblanks"))) {
            lint->parseOptions |= XML_PARSE_NOBLANKS;
#ifdef LIBXML_HTML_ENABLED
            lint->htmlOptions |= HTML_PARSE_NOBLANKS;
#endif
#ifdef LIBXML_OUTPUT_ENABLED
        } else if ((!strcmp(argv[i], "-o")) ||
                   (!strcmp(argv[i], "-output")) ||
                   (!strcmp(argv[i], "--output"))) {
            i++;
            lint->output = argv[i];
        } else if ((!strcmp(argv[i], "-format")) ||
                   (!strcmp(argv[i], "--format"))) {
            lint->format = 1;
            lint->parseOptions |= XML_PARSE_NOBLANKS;
#ifdef LIBXML_HTML_ENABLED
            lint->htmlOptions |= HTML_PARSE_NOBLANKS;
#endif
        } else if ((!strcmp(argv[i], "-encode")) ||
                   (!strcmp(argv[i], "--encode"))) {
            i++;
            lint->encoding = argv[i];
        } else if ((!strcmp(argv[i], "-pretty")) ||
                   (!strcmp(argv[i], "--pretty"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "pretty: missing integer value\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            if (parseInteger(&val, errStream, "pretty", argv[i],
                             0, 2) < 0)
                return(XMLLINT_ERR_UNCLASS);
            lint->format = val;
#ifdef LIBXML_ZLIB_ENABLED
        } else if ((!strcmp(argv[i], "-compress")) ||
                   (!strcmp(argv[i], "--compress"))) {
            lint->appOptions |= XML_LINT_ZLIB_COMPRESSION;
#endif
#ifdef LIBXML_C14N_ENABLED
        } else if ((!strcmp(argv[i], "-c14n")) ||
                   (!strcmp(argv[i], "--c14n"))) {
            lint->appOptions |= XML_LINT_CANONICAL_V1_0;
            lint->parseOptions |= XML_PARSE_NOENT | XML_PARSE_DTDATTR | XML_PARSE_DTDLOAD;
        } else if ((!strcmp(argv[i], "-c14n11")) ||
                   (!strcmp(argv[i], "--c14n11"))) {
            lint->appOptions |= XML_LINT_CANONICAL_V1_1;
            lint->parseOptions |= XML_PARSE_NOENT | XML_PARSE_DTDATTR | XML_PARSE_DTDLOAD;
        } else if ((!strcmp(argv[i], "-exc-c14n")) ||
                   (!strcmp(argv[i], "--exc-c14n"))) {
            lint->appOptions |= XML_LINT_CANONICAL_EXE;
            lint->parseOptions |= XML_PARSE_NOENT | XML_PARSE_DTDATTR | XML_PARSE_DTDLOAD;
#endif /* LIBXML_C14N_ENABLED */
#endif /* LIBXML_OUTPUT_ENABLED */
#ifdef LIBXML_READER_ENABLED
        } else if ((!strcmp(argv[i], "-stream")) ||
                   (!strcmp(argv[i], "--stream"))) {
             lint->appOptions |= XML_LINT_USE_STREAMING;
        } else if ((!strcmp(argv[i], "-walker")) ||
                   (!strcmp(argv[i], "--walker"))) {
             lint->appOptions |= XML_LINT_USE_WALKER;
             lint->noout = 1;
#ifdef LIBXML_PATTERN_ENABLED
        } else if ((!strcmp(argv[i], "-pattern")) ||
                   (!strcmp(argv[i], "--pattern"))) {
            i++;
            lint->pattern = argv[i];
#endif
#endif /* LIBXML_READER_ENABLED */
#ifdef LIBXML_SAX1_ENABLED
        } else if ((!strcmp(argv[i], "-sax1")) ||
                   (!strcmp(argv[i], "--sax1"))) {
            lint->parseOptions |= XML_PARSE_SAX1;
#endif /* LIBXML_SAX1_ENABLED */
        } else if ((!strcmp(argv[i], "-sax")) ||
                   (!strcmp(argv[i], "--sax"))) {
            lint->appOptions |= XML_LINT_SAX_ENABLED;
#ifdef LIBXML_RELAXNG_ENABLED
        } else if ((!strcmp(argv[i], "-relaxng")) ||
                   (!strcmp(argv[i], "--relaxng"))) {
            i++;
            lint->relaxng = argv[i];
            lint->parseOptions |= XML_PARSE_NOENT;
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
        } else if ((!strcmp(argv[i], "-schema")) ||
                 (!strcmp(argv[i], "--schema"))) {
            i++;
            lint->schema = argv[i];
            lint->parseOptions |= XML_PARSE_NOENT;
#endif
#ifdef LIBXML_SCHEMATRON_ENABLED
        } else if ((!strcmp(argv[i], "-schematron")) ||
                   (!strcmp(argv[i], "--schematron"))) {
            i++;
            lint->schematron = argv[i];
            lint->parseOptions |= XML_PARSE_NOENT;
#endif
        } else if ((!strcmp(argv[i], "-nonet")) ||
                   (!strcmp(argv[i], "--nonet"))) {
            lint->parseOptions |= XML_PARSE_NONET;
        } else if ((!strcmp(argv[i], "-nocompact")) ||
                   (!strcmp(argv[i], "--nocompact"))) {
            lint->parseOptions &= ~XML_PARSE_COMPACT;
#ifdef LIBXML_HTML_ENABLED
            lint->htmlOptions &= ~HTML_PARSE_COMPACT;
#endif
        } else if ((!strcmp(argv[i], "-load-trace")) ||
                   (!strcmp(argv[i], "--load-trace"))) {
            lint->appOptions |= XML_LINT_USE_LOAD_TRACE;
        } else if ((!strcmp(argv[i], "-path")) ||
                   (!strcmp(argv[i], "--path"))) {
            i++;
            parsePath(lint, BAD_CAST argv[i]);
#ifdef LIBXML_XPATH_ENABLED
        } else if ((!strcmp(argv[i], "-xpath")) ||
                   (!strcmp(argv[i], "--xpath"))) {
            i++;
            lint->noout++;
            lint->xpathquery = argv[i];
            lint->xpathsep = "\n";
        } else if ((!strcmp(argv[i], "-xpath0")) ||
                   (!strcmp(argv[i], "--xpath0"))) {
            i++;
            lint->noout++;
            lint->xpathquery = argv[i];
            lint->xpathsep = "\0";
#endif
        } else if ((!strcmp(argv[i], "-oldxml10")) ||
                   (!strcmp(argv[i], "--oldxml10"))) {
            lint->parseOptions |= XML_PARSE_OLD10;
        } else if ((!strcmp(argv[i], "-max-ampl")) ||
                   (!strcmp(argv[i], "--max-ampl"))) {
            i++;
            if (i >= argc) {
                fprintf(errStream, "max-ampl: missing integer value\n");
                return(XMLLINT_ERR_UNCLASS);
            }
            if (parseInteger(&val, errStream, "max-ampl", argv[i],
                             1, UINT_MAX) < 0)
                return(XMLLINT_ERR_UNCLASS);
            lint->maxAmpl = val;
        } else {
            fprintf(errStream, "Unknown option %s\n", argv[i]);
            usage(errStream, argv[0]);
            return(XMLLINT_ERR_UNCLASS);
        }
    }

    if (lint->appOptions & XML_LINT_NAVIGATING_SHELL)
        lint->repeat = 1;

    if ((lint->errorRingDump || (lint->errorRingDumpFile != NULL) ||
         (lint->errorRingDumpCborFile != NULL) ||
         (lint->errorRingDumpBinFile != NULL)) &&
        (lint->errorRingSize == 0)) {
        fprintf(errStream,
                "Warning: --error-ring-dump requires --error-ring; "
                "defaulting to 32\n");
        lint->errorRingSize = 32;
    }

    if (lint->errorJsonArray && !lint->errorJson) {
        lint->errorJson = 1;
    }

    if (lint->errorJsonFile != NULL) {
        lint->errorJsonStream = fopen(lint->errorJsonFile, "a");
        if (lint->errorJsonStream == NULL) {
            fprintf(errStream, "error-json-file: failed to open %s\n",
                    lint->errorJsonFile);
            return(XMLLINT_ERR_UNCLASS);
        }
    }

    if (lint->errorJsonWarnFile != NULL) {
        lint->errorJsonWarnStream = fopen(lint->errorJsonWarnFile, "a");
        if (lint->errorJsonWarnStream == NULL) {
            fprintf(errStream, "error-json-warn-file: failed to open %s\n",
                    lint->errorJsonWarnFile);
            if (lint->errorJsonStream != NULL)
                fclose(lint->errorJsonStream);
            return(XMLLINT_ERR_UNCLASS);
        }
    }

    if (lint->errorRingDumpFile != NULL) {
        lint->errorRingDumpStream = fopen(lint->errorRingDumpFile, "a");
        if (lint->errorRingDumpStream == NULL) {
            fprintf(errStream, "error-ring-dump-file: failed to open %s\n",
                    lint->errorRingDumpFile);
            if (lint->errorJsonStream != NULL)
                fclose(lint->errorJsonStream);
            if (lint->errorJsonWarnStream != NULL)
                fclose(lint->errorJsonWarnStream);
            return(XMLLINT_ERR_UNCLASS);
        }
        lint->errorRingDump = 1;
    }

    if (lint->errorRingDumpCborFile != NULL) {
        lint->errorRingDumpCborStream = fopen(lint->errorRingDumpCborFile, "ab");
        if (lint->errorRingDumpCborStream == NULL) {
            fprintf(errStream, "error-ring-dump-cbor-file: failed to open %s\n",
                    lint->errorRingDumpCborFile);
            if (lint->errorJsonStream != NULL)
                fclose(lint->errorJsonStream);
            if (lint->errorJsonWarnStream != NULL)
                fclose(lint->errorJsonWarnStream);
            if (lint->errorRingDumpStream != NULL)
                fclose(lint->errorRingDumpStream);
            return(XMLLINT_ERR_UNCLASS);
        }
        lint->errorRingDump = 1;
    }

    if (lint->errorRingDumpBinFile != NULL) {
        long pos;

        lint->errorRingDumpBinStream = fopen(lint->errorRingDumpBinFile, "ab");
        if (lint->errorRingDumpBinStream == NULL) {
            fprintf(errStream, "error-ring-dump-bin-file: failed to open %s\n",
                    lint->errorRingDumpBinFile);
            if (lint->errorJsonStream != NULL)
                fclose(lint->errorJsonStream);
            if (lint->errorJsonWarnStream != NULL)
                fclose(lint->errorJsonWarnStream);
            if (lint->errorRingDumpStream != NULL)
                fclose(lint->errorRingDumpStream);
            if (lint->errorRingDumpCborStream != NULL)
                fclose(lint->errorRingDumpCborStream);
            return(XMLLINT_ERR_UNCLASS);
        }
        if (fseek(lint->errorRingDumpBinStream, 0, SEEK_END) == 0) {
            pos = ftell(lint->errorRingDumpBinStream);
            if (pos == 0)
                xmllintWriteBinHeader(lint->errorRingDumpBinStream);
        }
    }

    if (lint->errorXmlFile != NULL) {
        lint->errorXmlStream = fopen(lint->errorXmlFile, "a");
        if (lint->errorXmlStream == NULL) {
            fprintf(errStream, "error-xml-file: failed to open %s\n",
                    lint->errorXmlFile);
            if (lint->errorJsonStream != NULL)
                fclose(lint->errorJsonStream);
            if (lint->errorJsonWarnStream != NULL)
                fclose(lint->errorJsonWarnStream);
            if (lint->errorRingDumpStream != NULL)
                fclose(lint->errorRingDumpStream);
            if (lint->errorRingDumpCborStream != NULL)
                fclose(lint->errorRingDumpCborStream);
            if (lint->errorRingDumpBinStream != NULL)
                fclose(lint->errorRingDumpBinStream);
            return(XMLLINT_ERR_UNCLASS);
        }
        lint->errorXml = 1;
    }

#ifndef _WIN32
    if (lint->errorSyslog) {
        openlog("xmllint", LOG_PID, lint->errorSyslogFacility);
    }
#endif

#ifdef LIBXML_READER_ENABLED
    if (lint->appOptions & XML_LINT_USE_STREAMING) {
        specialMode = "--stream";

        if (lint->appOptions & XML_LINT_SAX_ENABLED)
            xmllintOptWarnNoSupport(errStream, "--stream", "--sax");
#ifdef LIBXML_PUSH_ENABLED
        if (lint->appOptions & XML_LINT_PUSH_ENABLED)
            xmllintOptWarnNoSupport(errStream, "--stream", "--push");
#endif
#ifdef LIBXML_HTML_ENABLED
        if (lint->appOptions & XML_LINT_HTML_ENABLED)
            xmllintOptWarnNoSupport(errStream, "--stream", "--html");
#endif
    }
#endif /* LIBXML_READER_ENABLED */

    if (lint->appOptions & XML_LINT_SAX_ENABLED) {
        specialMode = "--sax";

#ifdef LIBXML_XINCLUDE_ENABLED
        if (lint->appOptions & XML_LINT_XINCLUDE)
            xmllintOptWarnNoSupport(errStream, "--sax", "--xinclude");
#endif
#ifdef LIBXML_RELAXNG_ENABLED
        if (lint->relaxng != NULL)
            xmllintOptWarnNoSupport(errStream, "--sax", "--relaxng");
#endif
    }

    if (specialMode != NULL) {
        if (lint->appOptions & XML_LINT_GENERATE)
            xmllintOptWarnNoSupport(errStream, specialMode, "--auto");
        if (lint->appOptions & XML_LINT_DROP_DTD)
            xmllintOptWarnNoSupport(errStream, specialMode, "--dropdtd");
        if (lint->appOptions & XML_LINT_NAVIGATING_SHELL)
            xmllintOptWarnNoSupport(errStream, specialMode, "--shell");
        if (lint->appOptions & XML_LINT_COPY_ENABLED)
            xmllintOptWarnNoSupport(errStream, specialMode, "--copy");
#ifdef LIBXML_XPATH_ENABLED
        if (lint->xpathquery != NULL)
            xmllintOptWarnNoSupport(errStream, specialMode, "--xpath");
#endif
#ifdef LIBXML_READER_ENABLED
        if (lint->appOptions & XML_LINT_USE_WALKER)
            xmllintOptWarnNoSupport(errStream, specialMode, "--walker");
#endif
#ifdef LIBXML_VALID_ENABLED
        if (lint->appOptions & XML_LINT_VALID_INSERTIONS)
            xmllintOptWarnNoSupport(errStream, specialMode, "--insert");
        if (lint->dtdvalid != NULL)
            xmllintOptWarnNoSupport(errStream, specialMode, "--dtdvalid");
        if (lint->dtdvalidfpi != NULL)
            xmllintOptWarnNoSupport(errStream, specialMode, "--dtdvalidfpi");
        if (lint->appOptions & XML_LINT_POST_VALIDATION)
            xmllintOptWarnNoSupport(errStream, specialMode, "--postvalid");
#endif
#ifdef LIBXML_SCHEMATRON_ENABLED
        if (lint->schematron != NULL)
            xmllintOptWarnNoSupport(errStream, specialMode, "--schematron");
#endif
#ifdef LIBXML_OUTPUT_ENABLED
        if (lint->output != NULL)
            xmllintOptWarnNoSupport(errStream, specialMode, "--output");
        if (lint->encoding != NULL)
            xmllintOptWarnNoSupport(errStream, specialMode, "--encode");
        if (lint->format > 0)
            xmllintOptWarnNoSupport(errStream, specialMode,
                                    "--format or -pretty");
#ifdef LIBXML_ZLIB_ENABLED
        if (lint->appOptions & XML_LINT_ZLIB_COMPRESSION)
            xmllintOptWarnNoSupport(errStream, specialMode, "--compress");
#endif
#ifdef LIBXML_HTML_ENABLED
        if (lint->appOptions & XML_LINT_XML_OUT)
            xmllintOptWarnNoSupport(errStream, specialMode, "--xmlout");
#endif
#ifdef LIBXML_C14N_ENABLED
        if (lint->appOptions & XML_LINT_CANONICAL_V1_0)
            xmllintOptWarnNoSupport(errStream, specialMode, "--c14n");
        if (lint->appOptions & XML_LINT_CANONICAL_V1_1)
            xmllintOptWarnNoSupport(errStream, specialMode, "--c14n11");
        if (lint->appOptions & XML_LINT_CANONICAL_EXE)
            xmllintOptWarnNoSupport(errStream, specialMode, "--exc-c14n");
#endif
#endif /* LIBXML_OUTPUT_ENABLED */
    }

#if defined(LIBXML_READER_ENABLED) && defined(LIBXML_PATTERN_ENABLED)
    if (lint->pattern && !((lint->appOptions & XML_LINT_USE_STREAMING) || (lint->appOptions & XML_LINT_USE_WALKER)))
        fprintf(errStream, "Warning: Option %s requires %s\n",
                "--pattern", "--stream or --walker");
#endif

#ifdef LIBXML_HTML_ENABLED
    if (lint->appOptions & XML_LINT_HTML_ENABLED) {
        if (lint->parseOptions & XML_PARSE_DTDATTR)
            xmllintOptWarnNoSupport(errStream, "--html", "--dtdattr");
        if (lint->parseOptions & XML_PARSE_DTDLOAD)
            xmllintOptWarnNoSupport(errStream, "--html", "--loaddtd");
        if (lint->maxAmpl)
            xmllintOptWarnNoSupport(errStream, "--html", "--max-ampl");
        if (lint->parseOptions & XML_PARSE_NOCDATA)
            xmllintOptWarnNoSupport(errStream, "--html", "--nocdata");
        if (lint->parseOptions & XML_PARSE_NODICT)
            xmllintOptWarnNoSupport(errStream, "--html", "--nodict");
        if (lint->parseOptions & XML_PARSE_NOENT)
            xmllintOptWarnNoSupport(errStream, "--html", "--noent");
        if (lint->parseOptions & XML_PARSE_NO_XXE)
            xmllintOptWarnNoSupport(errStream, "--html", "--no-xxe");
        if (lint->parseOptions & XML_PARSE_NONET)
            xmllintOptWarnNoSupport(errStream, "--html", "--nonet");
        if (lint->parseOptions & XML_PARSE_NSCLEAN)
            xmllintOptWarnNoSupport(errStream, "--html", "--nsclean");
        if (lint->parseOptions & XML_PARSE_OLD10)
            xmllintOptWarnNoSupport(errStream, "--html", "--oldxml10");
        if (lint->parseOptions & XML_PARSE_PEDANTIC)
            xmllintOptWarnNoSupport(errStream, "--html", "--pedantic");
        if (lint->parseOptions & XML_PARSE_DTDVALID)
            xmllintOptWarnNoSupport(errStream, "--html", "--valid");
        if (lint->parseOptions & XML_PARSE_SAX1)
            xmllintOptWarnNoSupport(errStream, "--html", "--sax1");
    } else {
        if (lint->htmlOptions & HTML_PARSE_NODEFDTD)
            fprintf(errStream, "Warning: Option %s requires %s\n",
                    "--nodefdtd", "--html");
#ifdef LIBXML_OUTPUT_ENABLED
        if (lint->appOptions & XML_LINT_XML_OUT)
            fprintf(errStream, "Warning: Option %s requires %s\n",
                    "--xmlout", "--html");
#endif
    }
#endif

    return(XMLLINT_RETURN_OK);
}

int
xmllintMain(int argc, const char **argv, FILE *errStream,
            xmlResourceLoader loader) {
    xmllintState state, *lint;
    int i, j, res;
    int files = 0;

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stderr), _O_BINARY);
#endif

    lint = &state;
    xmllintInit(lint);
    lint->errStream = errStream;
    lint->defaultResourceLoader = loader;

    res = xmllintParseOptions(lint, argc, argv);
    if (res != XMLLINT_RETURN_OK) {
        return(res);
    }

    if (lint->errorJson || lint->errorXml) {
        xmlSetStructuredErrorFunc(lint, xmllintStructuredError);
    }

    /*
     * Note that we must not make any memory allocations through xmlMalloc
     * before calling xmlMemSetup.
     */
    if (lint->maxmem != 0) {
        xmllintMaxmem = 0;
        xmllintMaxmemReached = 0;
        xmllintOom = 0;
        xmlMemSetup(myFreeFunc, myMallocFunc, myReallocFunc, myStrdupFunc);
    }

    LIBXML_TEST_VERSION

#ifdef LIBXML_CATALOG_ENABLED
    if ((lint->appOptions & XML_LINT_USE_NO_CATALOGS) != XML_LINT_USE_NO_CATALOGS) {
	if (lint->appOptions & XML_LINT_USE_CATALOGS) {
	    const char *catal;

	    catal = getenv("SGML_CATALOG_FILES");
	    if (catal != NULL) {
		xmlLoadCatalogs(catal);
	    } else {
		fprintf(errStream, "Variable $SGML_CATALOG_FILES not set\n");
	    }
	}
    }
#endif

#ifdef LIBXML_OUTPUT_ENABLED
    {
        const char *indent = getenv("XMLLINT_INDENT");
        if (indent != NULL) {
            lint->indentString = indent;
        }
    }
#endif

#ifdef LIBXML_SCHEMATRON_ENABLED
    if ((lint->schematron != NULL) && ((lint->appOptions & XML_LINT_SAX_ENABLED) != XML_LINT_SAX_ENABLED)
#ifdef LIBXML_READER_ENABLED
        && ((lint->appOptions & XML_LINT_USE_STREAMING) != XML_LINT_USE_STREAMING)
#endif /* LIBXML_READER_ENABLED */
	) {
	xmlSchematronParserCtxtPtr ctxt;

        /* forces loading the DTDs */
	lint->parseOptions |= XML_PARSE_DTDLOAD;
	if (lint->appOptions & XML_LINT_TIMINGS) {
	    startTimer(lint);
	}
	ctxt = xmlSchematronNewParserCtxt(lint->schematron);
        if (ctxt == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            goto error;
        }
	lint->wxschematron = xmlSchematronParse(ctxt);
	xmlSchematronFreeParserCtxt(ctxt);
	if (lint->wxschematron == NULL) {
	    fprintf(errStream, "Schematron schema %s failed to compile\n",
                    lint->schematron);
            lint->progresult = XMLLINT_ERR_SCHEMACOMP;
            goto error;
	}
	if (lint->appOptions & XML_LINT_TIMINGS) {
	    endTimer(lint, "Compiling the schemas");
	}
    }
#endif

#ifdef LIBXML_RELAXNG_ENABLED
    if ((lint->relaxng != NULL) && ((lint->appOptions & XML_LINT_SAX_ENABLED) != XML_LINT_SAX_ENABLED)
#ifdef LIBXML_READER_ENABLED
        && ((lint->appOptions & XML_LINT_USE_STREAMING) != XML_LINT_USE_STREAMING)
#endif /* LIBXML_READER_ENABLED */
	) {
	xmlRelaxNGParserCtxtPtr ctxt;

        /* forces loading the DTDs */
	lint->parseOptions |= XML_PARSE_DTDLOAD;
	if (lint->appOptions & XML_LINT_TIMINGS) {
	    startTimer(lint);
	}
	ctxt = xmlRelaxNGNewParserCtxt(lint->relaxng);
        if (ctxt == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            goto error;
        }
        xmlRelaxNGSetResourceLoader(ctxt, xmllintResourceLoader, lint);
	lint->relaxngschemas = xmlRelaxNGParse(ctxt);
	xmlRelaxNGFreeParserCtxt(ctxt);
	if (lint->relaxngschemas == NULL) {
	    fprintf(errStream, "Relax-NG schema %s failed to compile\n",
                    lint->relaxng);
            lint->progresult = XMLLINT_ERR_SCHEMACOMP;
            goto error;
	}
	if (lint->appOptions & XML_LINT_TIMINGS) {
	    endTimer(lint, "Compiling the schemas");
	}
    }
#endif /* LIBXML_RELAXNG_ENABLED */

#ifdef LIBXML_SCHEMAS_ENABLED
    if ((lint->schema != NULL)
#ifdef LIBXML_READER_ENABLED
        && ((lint->appOptions& XML_LINT_USE_STREAMING) != XML_LINT_USE_STREAMING)
#endif
	) {
	xmlSchemaParserCtxtPtr ctxt;

	if (lint->appOptions & XML_LINT_TIMINGS) {
	    startTimer(lint);
	}
	ctxt = xmlSchemaNewParserCtxt(lint->schema);
        if (ctxt == NULL) {
            lint->progresult = XMLLINT_ERR_MEM;
            goto error;
        }
        xmlSchemaSetResourceLoader(ctxt, xmllintResourceLoader, lint);
	lint->wxschemas = xmlSchemaParse(ctxt);
	xmlSchemaFreeParserCtxt(ctxt);
	if (lint->wxschemas == NULL) {
	    fprintf(errStream, "WXS schema %s failed to compile\n",
                    lint->schema);
            lint->progresult = XMLLINT_ERR_SCHEMACOMP;
            goto error;
	}
	if (lint->appOptions & XML_LINT_TIMINGS) {
	    endTimer(lint, "Compiling the schemas");
	}
    }
#endif /* LIBXML_SCHEMAS_ENABLED */

#if defined(LIBXML_READER_ENABLED) && defined(LIBXML_PATTERN_ENABLED)
    if ((lint->pattern != NULL) && ((lint->appOptions & XML_LINT_USE_WALKER) != XML_LINT_USE_WALKER)) {
        res = xmlPatternCompileSafe(BAD_CAST lint->pattern, NULL, 0, NULL,
                                    &lint->patternc);
	if (lint->patternc == NULL) {
            if (res < 0) {
                lint->progresult = XMLLINT_ERR_MEM;
            } else {
                fprintf(errStream, "Pattern %s failed to compile\n",
                        lint->pattern);
                lint->progresult = XMLLINT_ERR_SCHEMAPAT;
            }
            goto error;
	}
    }
#endif /* LIBXML_READER_ENABLED && LIBXML_PATTERN_ENABLED */

    /*
     * The main loop over input documents
     */
    for (i = 1; i < argc ; i++) {
        const char *filename = argv[i];
#if HAVE_DECL_MMAP
        int memoryFd = -1;
#endif

	if ((filename[0] == '-') && (strcmp(filename, "-") != 0)) {
            i += skipArgs(filename);
            continue;
        }

#if HAVE_DECL_MMAP
        if (lint->appOptions & XML_LINT_MEMORY) {
            struct stat info;
            if (stat(filename, &info) < 0) {
                lint->progresult = XMLLINT_ERR_RDFILE;
                break;
            }
            memoryFd = open(filename, O_RDONLY);
            if (memoryFd < 0) {
                lint->progresult = XMLLINT_ERR_RDFILE;
                break;
            }
            lint->memoryData = mmap(NULL, info.st_size, PROT_READ,
                                    MAP_SHARED, memoryFd, 0);
            if (lint->memoryData == (void *) MAP_FAILED) {
                close(memoryFd);
                fprintf(errStream, "mmap failure for file %s\n", filename);
                lint->progresult = XMLLINT_ERR_RDFILE;
                break;
            }
            lint->memorySize = info.st_size;
        }
#endif /* HAVE_DECL_MMAP */

	if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat > 1))
	    startTimer(lint);

#ifdef LIBXML_READER_ENABLED
        if (lint->appOptions & XML_LINT_USE_STREAMING) {
            for (j = 0; j < lint->repeat; j++)
                streamFile(lint, filename);
        } else
#endif /* LIBXML_READER_ENABLED */
        {
            xmlParserCtxtPtr ctxt;

#ifdef LIBXML_HTML_ENABLED
            if (lint->appOptions & XML_LINT_HTML_ENABLED) {
#ifdef LIBXML_PUSH_ENABLED
                if (lint->appOptions & XML_LINT_PUSH_ENABLED) {
                    ctxt = htmlCreatePushParserCtxt(NULL, NULL, NULL, 0,
                                                    filename,
                                                    XML_CHAR_ENCODING_NONE);
                } else
#endif /* LIBXML_PUSH_ENABLED */
                {
                    ctxt = htmlNewParserCtxt();
                }
                htmlCtxtUseOptions(ctxt, lint->htmlOptions);
            } else
#endif /* LIBXML_HTML_ENABLED */
            {
#ifdef LIBXML_PUSH_ENABLED
                if (lint->appOptions & XML_LINT_PUSH_ENABLED) {
                    ctxt = xmlCreatePushParserCtxt(NULL, NULL, NULL, 0,
                                                   filename);
                } else
#endif /* LIBXML_PUSH_ENABLED */
                {
                    ctxt = xmlNewParserCtxt();
                }
                xmlCtxtUseOptions(ctxt, lint->parseOptions);
            }
            if (ctxt == NULL) {
                lint->progresult = XMLLINT_ERR_MEM;
                goto error;
            }

            if (lint->errorJson || lint->errorXml)
                xmlCtxtSetErrorHandler(ctxt, xmllintStructuredError, lint);
            if (lint->errorRingSize > 0)
                xmlCtxtSetErrorRingSize(ctxt, lint->errorRingSize);
            if (lint->errorDedupLimit > 0)
                xmlCtxtSetErrorDedup(ctxt, lint->errorDedupLimit);

            if (lint->appOptions & XML_LINT_SAX_ENABLED) {
                const xmlSAXHandler *handler;

                if (lint->noout) {
                    handler = &emptySAXHandler;
#ifdef LIBXML_SAX1_ENABLED
                } else if (lint->parseOptions & XML_PARSE_SAX1) {
                    handler = &debugSAXHandler;
#endif
                } else {
                    handler = &debugSAX2Handler;
                }

                *ctxt->sax = *handler;
                ctxt->userData = lint;
            }

            xmlCtxtSetResourceLoader(ctxt, xmllintResourceLoader, lint);
            if (lint->maxAmpl > 0)
                xmlCtxtSetMaxAmplification(ctxt, lint->maxAmpl);

            lint->ctxt = ctxt;

            for (j = 0; j < lint->repeat; j++) {
                if (j > 0) {
#ifdef LIBXML_PUSH_ENABLED
                    if (lint->appOptions & XML_LINT_PUSH_ENABLED) {
                        xmlCtxtResetPush(ctxt, NULL, 0, NULL, NULL);
                    } else
#endif
                    {
                        xmlCtxtReset(ctxt);
                    }
                    if (lint->errorRingSize > 0)
                        xmlCtxtResetErrorRing(ctxt);
                    if (lint->errorDedupLimit > 0)
                        xmlCtxtResetErrorDedup(ctxt);
                }

                lint->errorJsonArrayOpen = 0;
                lint->errorJsonArrayCount = 0;
                lint->errorJsonArrayFile = filename;
                lint->errorJsonCount = 0;
                xmllintErrorStatsReset(lint);
                if (lint->errorJsonChecksum) {
                    lint->errorChecksum = xmllintChecksumFile(filename,
                                                              &lint->checksumValid);
                } else {
                    lint->errorChecksum = 0;
                    lint->checksumValid = 0;
                }
                if (lint->errorJsonArray) {
                    xmllintStartErrorArray(lint, filename);
                }

                if (lint->appOptions & XML_LINT_SAX_ENABLED) {
                    testSAX(lint, filename);
                } else {
                    parseAndPrintFile(lint, filename);
                }
            }

            if (lint->errorJsonArray && lint->errorJsonArrayOpen) {
                xmllintFinishErrorArray(lint);
            } else if (lint->errorJsonSummary && lint->errorJson) {
                xmllintWriteSummaryLine(lint, filename);
            }

            if (lint->errorRingDump)
                xmllintDumpErrorRing(lint, ctxt, filename);
            if (lint->errorRingDumpCborStream != NULL)
                xmllintDumpErrorRingCbor(lint, ctxt, filename);
            if (lint->errorRingDumpBinStream != NULL)
                xmllintDumpErrorRingBinary(lint, ctxt, filename);
            xmlFreeParserCtxt(ctxt);
        }

        if ((lint->appOptions & XML_LINT_TIMINGS) && (lint->repeat > 1)) {
            endTimer(lint, "%d iterations", lint->repeat);
        }

        files += 1;

#if HAVE_DECL_MMAP
        if (lint->appOptions & XML_LINT_MEMORY) {
            munmap(lint->memoryData, lint->memorySize);
            close(memoryFd);
        }
#endif
    }

    if (lint->appOptions & XML_LINT_GENERATE)
	parseAndPrintFile(lint, NULL);

    if ((files == 0) && ((lint->appOptions & XML_LINT_GENERATE) != XML_LINT_GENERATE) && (lint->version == 0)) {
	usage(errStream, argv[0]);
        lint->progresult = XMLLINT_ERR_UNCLASS;
    }

error:

#ifdef LIBXML_SCHEMATRON_ENABLED
    if (lint->wxschematron != NULL)
	xmlSchematronFree(lint->wxschematron);
#endif
#ifdef LIBXML_RELAXNG_ENABLED
    if (lint->relaxngschemas != NULL)
	xmlRelaxNGFree(lint->relaxngschemas);
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
    if (lint->wxschemas != NULL)
	xmlSchemaFree(lint->wxschemas);
#endif
#if defined(LIBXML_READER_ENABLED) && defined(LIBXML_PATTERN_ENABLED)
    if (lint->patternc != NULL)
        xmlFreePattern(lint->patternc);
#endif

    xmlCleanupParser();

    if ((lint->maxmem) && (xmllintMaxmemReached)) {
        fprintf(errStream, "Maximum memory exceeded (%d bytes)\n",
                xmllintMaxmem);
    } else if (lint->progresult == XMLLINT_ERR_MEM) {
        fprintf(errStream, "Out-of-memory error reported\n");
    }

    if (lint->errorJsonArray && lint->errorJsonArrayOpen) {
        xmllintFinishErrorArray(lint);
    }

    if (lint->errorJsonStream != NULL)
        fclose(lint->errorJsonStream);
    if (lint->errorJsonWarnStream != NULL)
        fclose(lint->errorJsonWarnStream);
    if (lint->errorRingDumpStream != NULL)
        fclose(lint->errorRingDumpStream);
    if (lint->errorRingDumpCborStream != NULL)
        fclose(lint->errorRingDumpCborStream);
    if (lint->errorRingDumpBinStream != NULL)
        fclose(lint->errorRingDumpBinStream);
    if (lint->errorXmlStream != NULL)
        fclose(lint->errorXmlStream);
#ifndef _WIN32
    if (lint->errorSyslog)
        closelog();
#endif
    if (lint->errorCodeCounts != NULL)
        xmlFree(lint->errorCodeCounts);
    if (lint->errorStageCounts != NULL) {
        int stageIndex;
        for (stageIndex = 0; stageIndex < lint->errorStageCountsCount;
             stageIndex++) {
            xmlFree(lint->errorStageCounts[stageIndex].name);
            lint->errorStageCounts[stageIndex].name = NULL;
        }
        xmlFree(lint->errorStageCounts);
    }

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    if ((lint->maxmem) &&
        (xmllintOom != (lint->progresult == XMLLINT_ERR_MEM))) {
        fprintf(stderr, "xmllint: malloc failure %s reported\n",
                xmllintOom ? "not" : "erroneously");
        abort();
    }
#endif

    return(lint->progresult);
}
