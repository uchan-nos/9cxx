#!/bin/sh

if [ $# -ne 4 ]
then
    echo "Usage: $0 TESTCASE COMPILE_CODE EXPECTED_CODE EXPECTED_OUT"
    exit 1
fi

TESTCASE="$1"
COMPILE_CODE=$2
EXPECTED_CODE=$3
EXPECTED_OUT="$4"

CXX=$(dirname $0)/../src/9cxx

if [ -f "$TESTCASE.cpp" ]
then
    TESTCASE=$(cat "$TESTCASE.cpp")
fi

if [ "$QUIET" = "y" ]
then
    echo "$TESTCASE" | $CXX 2>/dev/null > $(dirname $0)/testcase.s
else
    echo "$TESTCASE" | $CXX > $(dirname $0)/testcase.s
fi
COMPILE_ACTUAL_CODE=$?
if [ $COMPILE_ACTUAL_CODE -ne $COMPILE_CODE ]
then
    echo "[FAILED] testcase $TESTCASE"
    echo "  Actual compile code $COMPILE_ACTUAL_CODE, expected $COMPILE_CODE"
    exit 255
fi
if [ $COMPILE_ACTUAL_CODE -ne 0 ] && [ $COMPILE_ACTUAL_CODE -eq $COMPILE_CODE ]
then
    echo "[  OK  ] testcase $TESTCASE"
    exit 0
fi

as $(dirname $0)/testcase.s -o $(dirname $0)/testcase.o
clang++ $(dirname $0)/testcase.o $(dirname $0)/supplement.cpp -o $(dirname $0)/testcase.out

$(dirname $0)/testcase.out > $(dirname $0)/actual.out
ACTUAL_CODE=$?

if [ ! -f "$EXPECTED_OUT" ]
then
    echo -n "$EXPECTED_OUT" > $(dirname $0)/expected.out
    EXPECTED_OUT=$(dirname $0)/expected.out
fi
DIFF=$(diff $(dirname $0)/actual.out $EXPECTED_OUT)

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
    echo "  Actual out $(cat $(dirname $0)/actual.out), expected $EXPECTED_OUT"
    exit 255
fi

exit 0
