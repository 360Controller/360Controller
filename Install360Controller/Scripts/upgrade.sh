#!/bin/sh
echo "installer got 0: '$0' 1: '$1' 2: '$2' 3: '$3'"
cd $3
cd System/Library/Extensions
kextload 360Controller.kext
kextunload 360Controller.kext
# If we can't unload the driver, the user risks kernel panics (not anymore)
exit 0
