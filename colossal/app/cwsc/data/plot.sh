#!/bin/sh

TYPE=MAP
COL1=14
COL2=15
INTERVAL=5000
INPUT=part-00000
OUTPUT=running

TMP1=tmp1
TMP2=tmp2

awk -F'\t' "{ if (\$13 == \"$TYPE\") {print \$$COL1, 1; print \$$COL2, -1;} }" $INPUT > $TMP1

sort -gk1 -t\  $TMP1 > $TMP2

awk -F' ' "BEGIN{ n = 0; } { n += \$2; m = n; if (m < 0) m = 0; print \$1, m; }" $TMP2 > $TMP1

awk -F' ' "{ if (last == 0 || \$1 - last > $INTERVAL) { last += $INTERVAL; print \$0; } } END{print}" $TMP1 > $OUTPUT

rm -rf $TMP1 $TMP2
