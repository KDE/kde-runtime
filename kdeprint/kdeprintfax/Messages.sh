#! /usr/bin/env bash
$XGETTEXT `find . -name \*.h -o -name \*.cpp -o -name \*.cc` -o $podir/kdeprintfax.pot
