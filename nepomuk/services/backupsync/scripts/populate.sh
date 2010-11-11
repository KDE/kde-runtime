#!/bin/bash

TIMES=10
if [ $# -gt 0 ]; then
	TIMES=$1
fi

i=0
for (( i=0; i < $TIMES; i++ )) 
do 
	touch $i
done;