#!/bin/bash
if [ $# -lt 1 ]; then
    echo "Usage: $0 filename"
    exit
fi

source ~/kde/dev/kde4dev

# Get correct FileName
DIR=`dirname $0`
FILE=$1

# Combine with the template
cat $DIR/template.cpp > $DIR/main.cpp
cat $FILE >> $DIR/main.cpp
echo "return 0; }" >> $DIR/main.cpp

WD=`pwd`
cd $DIR/
cb
# Compile
cmakekde
make

# Run
./nepomuk-test

cd $WD
