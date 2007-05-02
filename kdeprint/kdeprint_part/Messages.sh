#! /usr/bin/env bash
$EXTRACTRC kdeprint_part.rc >> rc.cpp || exit 11
$XGETTEXT *.cpp -o $podir/kdeprint_part.pot
