#!/bin/bash

wd=`pwd`

echo "Removing everything but the backup .."

cd $KDEHOME/share/apps/nepomuk
rm -rf repository
cd backupsync
rm -rf log

echo "Removed!"

cd $wd

