#!/bin/sh -e

RUNNER=$(dirname $0)/run_testcase.sh
export QUIET=y

# $RUNNER
#    param 1: target code
#    param 2: expected compiler's exit code
#    param 3: expected target program's exit code
#    param 4: expected target program's output
$RUNNER "1 + 2;" 0 3 ""
$RUNNER "1+;" 255 0 ""
$RUNNER "5/2;" 0 2 ""
$RUNNER "(1-3)*(1+3);" 0 248 ""
$RUNNER "24==3*2*4;" 0 1 ""
$RUNNER "{int f42();f42();}" 0 42 ""
$RUNNER "{int v,add();v=2;add(add(1,v),v*4);}" 0 11 ""
