#!/bin/sh
cd $3
cd System/Library/Extensions
kextunload 360Controller.kext
# If we can't unload the driver, the user risks kernel panics
exit $?
