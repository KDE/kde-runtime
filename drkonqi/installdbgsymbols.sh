#!/bin/bash
# Interface to install debug symbols packages - Dario Andres Rodriguez 2009
# Distributions have to modify/adapt this script to their needs/naming/tools
# 
# Parameters this script gets:
# $1 is the (crashed) program executable which requires debug symbols (ex. plasma-desktop)
# $2... libraries with missing debug symbols (full path, extracted from backtrace)
# FIXME, may be we need some better param organization here?
#
# Return values:
# 0 - The missing debug packages were installed succesfuly
# 10 - The missing debug symbols packages are not present
# 20 - Feature not implemented in the distro (script not modified, no debug packages at all)
# 30 - The missing debug symbols packages are present but there was an error, and there were not installed
# (TODO Am I missing some other possibility?)
#
# Note: distro tools should do the work to get permissions and/or show progress/download UI
#
#Implement package install distro-dependant logic here: -------

exit 20; #Default return value, not implemented
