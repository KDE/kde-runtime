#! /usr/bin/env bash
$EXTRACTRC `find . -name "*.ui" -o -name "*.kcfg"` >> rc.cpp
$XGETTEXT `find . -name "*.cpp"` -o $podir/nepomukcontroller.pot
rm -rf rc.cpp
