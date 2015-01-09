#!/bin/sh

#This is to fix a bug in Yosemite.
/bin/ln -s /Library/Extensions/360Controller.kext /System/Library/Extensions/

/usr/bin/touch /System/Library/Extensions
/usr/bin/touch /Library/Extensions
/bin/launchctl load -w /Library/LaunchDaemons/com.mice.360Daemon.plist

exit 0
