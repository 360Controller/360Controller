#!/bin/sh

/usr/bin/touch /System/Library/Extensions
/bin/launchctl load -w /Library/LaunchDaemons/com.mice.360Daemon.plist

exit 0
