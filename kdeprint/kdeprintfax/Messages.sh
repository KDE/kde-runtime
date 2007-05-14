#! /usr/bin/env bash
$EXTRACTRC *.rc *.ui *.kcfg > rc.cpp
$XGETTEXT `find . -name \*.h -o -name \*.cpp -o -name \*.cc` -o $podir/kdeprintfax.pot
