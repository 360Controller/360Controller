#!/bin/sh
cd $3
cd System/Library/Extensions
kextunload 360Controller.kext
#Fail if didn't unload? Currently, don't.
#exit $?
exit 0
