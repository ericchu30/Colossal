#!/bin/sh

TOTAL=9101
TYPE=MAP
COL1=5
COL2=7
INTERVAL=1000
INPUT=workload
OUTPUT=plot.txt

TMP1=tmp1
TMP2=tmp2

awk -F'\t' "{ if (\$4 == \"$TYPE\") {print \$$COL1, 1; print \$$COL2, -1;} }" $INPUT > $TMP1

sort -gk1 -t\  $TMP1 > $TMP2

awk -F' ' "BEGIN{ n = 0; } { n += \$2; m = n; if (m < 0) m = 0; print \$1, m; }" $TMP2 > $TMP1

awk -F' ' "{ if (last == 0 || \$1 - last > $INTERVAL) { last += $INTERVAL; print \$0; } } END{print}" $TMP1 > $OUTPUT

rm -rf $TMP1 $TMP2
