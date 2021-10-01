#! /bin/sh

failed=0

printf "hello..."
output="$(./scrawl examples/hello.s)"
if [ "$output" = "Hello" ]
then
    echo "pass"
else
    echo "FAIL"
    echo "Expected: \"Hello\"" >&2
    echo "Got:      \"$output\"" >&2
    failed=$((failed + 1))
fi

printf "comment..."
output="$(./scrawl examples/comment.s < examples/comment.s.in | diff -u examples/comment.s -)"
s=$?
if [ $s -eq 0 ]
then
    echo "pass"
else
    echo "FAIL"
    echo "$output" >&2
    failed=$((failed + 1))
fi

printf "string..."
output="$(echo "msg:    byte \"test\"" | ./scrawl examples/string.s)"
expected_output="msg:    byte \$74 \$65 \$73 \$74 "
if [ "$output" = "$expected_output" ]
then
    echo "pass"
else
    echo "FAIL"
    echo "Expected: \"$expected_output\"" >&2
    echo "Got:      \"$output\"" >&2
    failed=$((failed + 1))
fi

printf "const..."
output="$(./scrawl examples/string.s < examples/hello.s.in \
        | ./scrawl examples/const.s | diff -u examples/hello.s -)"
s=$?
if [ $s -eq 0 ]
then
    echo "pass"
else
    echo "FAIL"
    echo "$output" >&2
    failed=$((failed + 1))
fi

printf "echo..."
output="$(./scrawl examples/echo.s a b cde 12)"
expected_output="examples/echo.sabcde12"
if [ "$output" = "$expected_output" ]
then
    echo "pass"
else
    echo "FAIL"
    echo "Expected: \"$expected_output\"" >&2
    echo "Got:      \"$output\"" >&2
    failed=$((failed + 1))
fi

if [ "$failed" -eq 0 ]
then
    echo "All tests passed"
else
    echo "$failed test failures"
    exit 2
fi
