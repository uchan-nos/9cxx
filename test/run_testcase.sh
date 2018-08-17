#!/bin/sh

if [ $# -ne 3 ]
then
    echo "Usage: $0 TESTCASE EXPECTED_CODE EXPECTED_OUT"
    exit 1
fi

TESTCASE=$1
EXPECTED_CODE=$2
EXPECTED_OUT=$3

CXX=$(dirname $0)/../src/9cxx

if [ -f $TESTCASE.cpp ]
then
    cat $TESTCASE.cpp | $CXX > $TESTCASE.s
else
    echo "$TESTCASE" | $CXX > $TESTCASE.s
fi
clang++ $TESTCASE.s

./a.out > actual.out
ACTUAL_CODE=$?

if [ ! -f "$EXPECTED_OUT" ]
then
    echo -n "$EXPECTED_OUT" > expected.out
    EXPECTED_OUT=expected.out
fi
DIFF=$(diff actual.out $EXPECTED_OUT)

TEST_FAILED=0
if [ "$DIFF" != "" ]
then
    TEST_FAILED=1
fi

if [ $ACTUAL_CODE -ne $EXPECTED_CODE ]
then
    TEST_FAILED=1
fi

if [ $TEST_FAILED -eq 0 ]
then
    echo "[  OK  ] testcase $TESTCASE"
else
    echo "[FAILED] testcase $TESTCASE"
    echo "  Actual code $ACTUAL_CODE, expected $EXPECTED_CODE"
    echo "  Actual out $(cat actual.out), expected $EXPECTED_OUT"
fi

