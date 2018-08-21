#!/bin/sh -e

RUNNER=$(dirname $0)/run_testcase.sh
export QUIET=y

# $RUNNER
#    param 1: target code
#    param 2: expected compiler's exit code
#    param 3: expected target program's exit code
#    param 4: expected target program's output
$RUNNER "int main(){1 + 2;}" 0 3 ""
$RUNNER "int main(){1+;}" 255 0 ""
$RUNNER "int main(){5/2;}" 0 2 ""
$RUNNER "int main(){(1-3)*(1+3);}" 0 248 ""
$RUNNER "int main(){24==3*2*4;}" 0 1 ""
$RUNNER "int main(){int f42();f42();}" 0 42 ""
$RUNNER "int main(){int v,add();v=2;add(add(1,v),v*4);}" 0 11 ""
$RUNNER "int f3(){3;} int f42(); int main(){int add(); add(f3(),f42());}" 0 45 ""
