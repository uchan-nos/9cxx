#!/bin/sh -e

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
$RUNNER "{int a,b;a=1;b=2;a+b*3;}" 0 7 ""
$RUNNER "{int f42();f42();}" 0 42 ""
$RUNNER "{int v,add();v=2;add(3,v*4);}" 0 11 ""
$RUNNER "{int add();add(add(1,1),add(2,3));}" 0 7 ""
