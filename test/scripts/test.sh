#!/bin/sh

set -e

echo "## Scripts regression tests"

if [ -n "$1" ]; then
    xmllint=$1
else
    xmllint=./xmllint
fi

exitcode=0

for i in test/scripts/*.script ; do
    name=$(basename $i .script)
    xml="./test/scripts/$name.xml"

    if [ -f $xml ] ; then
        if [ ! -f result/scripts/$name ] ; then
            echo "New test file $name"

            $xmllint --shell $xml < $i \
                > result/scripts/$name \
                2> result/scripts/$name.err
        else
            $xmllint --shell $xml < $i > shell.out 2> shell.err || true

            if [ -f result/scripts/$name.err ]; then
                resulterr="result/scripts/$name.err"
            else
                resulterr=/dev/null
            fi

            log=$(
                diff -u result/scripts/$name shell.out || true;
                diff -u $resulterr shell.err || true
            )

            if [ -n "$log" ] ; then
                echo $name result
                echo "$log"
                exitcode=1
            fi

            rm shell.out shell.err
        fi
    fi
done

echo "## Error output regression tests"

err_xml="test/scripts/error_xml_redact.xml"
err_out="error_xml_redact.err"
err_tmp="error_xml_redact.out"

$xmllint --noout --error-xml --error-redact all "$err_xml" \
    > /dev/null 2> "$err_tmp" || true

if [ ! -f result/scripts/$err_out ]; then
    echo "New test file $err_out"
    cp "$err_tmp" result/scripts/$err_out
else
    log=$(diff -u result/scripts/$err_out "$err_tmp" || true)
    if [ -n "$log" ] ; then
        echo "$err_out result"
        echo "$log"
        exitcode=1
    fi
fi

rm -f "$err_tmp"

bin_tmp="error_ring_dump.bin"
rm -f "$bin_tmp"
$xmllint --noout --error-ring 2 --error-ring-dump-bin-file "$bin_tmp" \
    --error-redact file "$err_xml" > /dev/null 2> /dev/null || true

if [ ! -f "$bin_tmp" ]; then
    echo "error_ring_dump.bin missing"
    exitcode=1
else
    header=$(dd if="$bin_tmp" bs=1 count=4 2>/dev/null)
    if [ "$header" != "XERB" ]; then
        echo "error_ring_dump.bin header mismatch"
        exitcode=1
    fi
fi

rm -f "$bin_tmp"

exit $exitcode
