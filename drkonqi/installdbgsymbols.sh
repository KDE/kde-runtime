#!/bin/bash
# Interface to install debug symbols packages
# Distributions have to modify/adapt this script to their needs/naming/tools
# 
# Parameters this script gets:
# $1 is the (crashed) program executable which requires debug symbols (ex. plasma-desktop)
#
# Return values:
# 0 - The missing debug packages were installed succesfuly
# 10 - The missing debug symbols packages are not present
# 20 - Feature not implemented in the distro
# 30 - The missing debug symbols packages are present but there was an error, and there were not installed
#
# Note: distro tools should do the work to get permissions
#
#Implement package install distro-dependant logic here: -------

exit 20; #Default return value, not implemented
