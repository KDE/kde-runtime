#!/bin/bash

#
# Converts the syncFile into a more human readable format
#

if [ $# -lt 1 ]; then
    echo "Correct usage is \"$0 filename\""
    exit
fi


tar --format=posix -xvf $1 > /dev/null

RESOURCES=`cat identificationfile | grep -P -i -o "<nepomuk:/res/[\w\d-]*>" | uniq`

COUNT=0
for RES in $RESOURCES; do
    OUTPUT="<nepomuk:/res/Res${COUNT}>"
    echo "$RES ----> $OUTPUT"
    sed -i "s#${RES}#${OUTPUT}#g" identificationfile
    sed -i "s#${RES}#${OUTPUT}#g" logfile
    let "COUNT+=1"
done

# Create tar file
tar --format=posix -cvf $1 logfile identificationfile > /dev/null

# Remove unnecessary files
rm logfile
rm identificationfile
