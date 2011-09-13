#! /usr/bin/env bash
$XGETTEXT `find . -name "*.cpp" | grep -v '/test/'` -o $podir/nepomukstorage.pot
