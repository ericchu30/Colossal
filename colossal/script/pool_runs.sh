#!/bin/bash

if [ "$#" -ne "2" ]; then
    echo "usage: $0 type pool";
    exit -1;
fi

grep "	$1	" | grep "$2" | sort -gk8 -t\	 | cut -f2 -d\	  | uniq -c | sed -E 's/^ +//g' | cut -f2 -d\  | sort | uniq -c
