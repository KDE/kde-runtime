#! /usr/bin/env bash
$EXTRACTRC `find . -name "*.ui"` >> rc.cpp || exit 11
$XGETTEXT `find . -name "*.cpp" | grep -v '/kioslaves/'` -o $podir/nepomuk.pot
rm -f rc.cpp
