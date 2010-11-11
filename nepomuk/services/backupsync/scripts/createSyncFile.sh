#!/bin/bash

IDENTFILE=identificationfile
LOGFILE=logfile

if [ $# -lt 2 ]; then
    echo "Correct usage - $0 outputFile"
    echo "or $0 outFile LogFile IdentificationFile"
    exit
fi

if [ $# -lt 4 ]; then
    LOGFILE=$2
    IDENTFILE=$3
fi

tar --format=posix -cvf $1 $LOGFILE $IDENTFILE