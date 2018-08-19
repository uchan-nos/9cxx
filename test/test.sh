#!/bin/sh

RUNNER=$(dirname $0)/run_testcase.sh
export QUIET=y

# $RUNNER
#    param 1: target code
#    param 2: expected compiler's exit code
#    param 3: expected target program's exit code
#    param 4: expected target program's output
$RUNNER "1;" 0 1 ""
$RUNNER "3;" 0 3 ""
$RUNNER "1 + 2;" 0 3 ""
$RUNNER "1+;" 255 0 ""
$RUNNER "3-1;" 0 2 ""
$RUNNER "1-2;" 0 255 ""
$RUNNER "2*3;" 0 6 ""
$RUNNER "5/2;" 0 2 ""
$RUNNER "6*;" 255 0 ""
$RUNNER "(3-1)*(1+3);" 0 8 ""
$RUNNER "24==3*2*4;" 0 1 ""
$RUNNER "2!=2;" 0 0 ""
$RUNNER "{}" 0 0 ""
$RUNNER "{1+1;2+3;}" 0 5 ""
$RUNNER "{a=1;a+1;}" 0 2 ""
