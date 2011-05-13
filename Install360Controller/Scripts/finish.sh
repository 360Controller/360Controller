#!/bin/sh
##installdir="$3"
##newer=`ps auxw | grep kextd | grep -v grep | wc -l`
##if [ $newer -eq 0 ]
##then
##cd "$installdir"
##cd System/Library/Extensions
##kextload 360Controller.kext
##else
#/bin/killall -1 kextd
##fi

cd "$3"
cd System/Library/Extensions
/sbin/kextload 360Controller.kext

exit 0
