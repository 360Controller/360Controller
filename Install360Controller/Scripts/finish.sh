#!/bin/sh
installdir="$3"
newer=`ps -ax | awk '{print $1" "$5}' | grep kextd | awk '{print $1}'`
if [ "$newer" == "" ]
then
cd "$installdir"
cd System/Library/Extensions
kextload 360Controller.kext
else
/bin/kill -1 $newer
fi
exit $?
