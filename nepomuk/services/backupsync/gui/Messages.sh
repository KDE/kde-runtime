#! /usr/bin/env bash
$EXTRACTRC `find . -name "*.ui"` >> rc.cpp
$XGETTEXT `find . -name "*.cpp"` -o $podir/nepomukbackup.pot
rm -rf rc.cpp
