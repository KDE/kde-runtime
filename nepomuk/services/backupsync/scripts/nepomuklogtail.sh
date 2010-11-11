#!/bin/bash

TFILE="/tmp/nepomuklog"
FILES="`find $KDEHOME/share/apps/nepomuk/backupsync/log/* -type f`"
for i in $FILES; do
  cat "$i" >> "$TFILE"
done

tail $TFILE $@
rm "$TFILE"
