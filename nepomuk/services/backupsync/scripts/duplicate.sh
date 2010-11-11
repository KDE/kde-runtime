#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage : $0 filename numberOfFiles"
fi

FILENAME=$1
NUM=$2

for (( i=0; i<$NUM; i++ )); do
    echo "$i"
    cp $FILENAME $i
done;