#! /usr/bin/env bash
$EXTRACTRC `find . -name "*.ui"` >> rc.cpp || exit 11
$XGETTEXT -ktranslate:1,1t -ktranslate:1c,2,2t *.cpp -o $podir/kcmlocale.pot
$XGETTEXT TIMEZONES rc.cpp -o $podir/../kdelibs/timezones4.pot
