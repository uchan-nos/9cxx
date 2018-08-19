#!/bin/sh

RUNNER=$(dirname $0)/run_testcase.sh
export QUIET=y

# $RUNNER
#    param 1: target code
#    param 2: expected compiler's exit code
#    param 3: expected target program's exit code
#    param 4: expected target program's output
$RUNNER "1" 0 1 ""
$RUNNER "3" 0 3 ""
$RUNNER "1 + 2" 0 3 ""
$RUNNER "1+" 255 0 ""
$RUNNER "3-1" 0 2 ""
$RUNNER "1-2" 0 255 ""
