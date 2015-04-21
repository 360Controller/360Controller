#!/bin/sh

# Clear out daemon

# Startup Items is deprecated, should this be removed?
# the daemon is already been launched using launchctl and this folder doent appear to be created anymore.
# https://developer.apple.com/library/mac/documentation/MacOSX/Conceptual/BPSystemStartup/Chapters/StartupItems.html
if [ -d /Library/StartupItems/360ControlDaemon ]; then
   /bin/rm -r /Library/StartupItems/360ControlDaemon
fi

if [ -f /Library/LaunchDaemons/com.mice.360Daemon.plist ]; then
   launchctl stop com.mice.360Daemon
   launchctl unload /Library/LaunchDaemons/com.mice.360Daemon.plist
   /bin/rm /Library/LaunchDaemons/com.mice.360Daemon.plist
fi

# this folder doesnt appear to be created in recent versions too
if [ -d /Library/Application\ Support/MICE/360Daemon ]; then
   /bin/rm -r /Library/Application\ Support/MICE/360Daemon
fi

if [ -d /Library/Application\ Support/MICE/360Daemon.app ]; then
   /bin/rm -r /Library/Application\ Support/MICE/360Daemon.app
fi

# Remove preference pane

if [ -d /Library/PreferencePanes/Pref360Control.prefPane ]; then
   /bin/rm -r /Library/PreferencePanes/Pref360Control.prefPane
fi

# Remove drivers

if [ -d /System/Library/Extensions/360Controller.kext ]; then
   kextunload /System/Library/Extensions/360Controller.kext
   /bin/rm -r /System/Library/Extensions/360Controller.kext
fi

if [ -d /System/Library/Extensions/Wireless360Controller.kext ]; then
   kextunload /System/Library/Extensions/Wireless360Controller.kext
   /bin/rm -r /System/Library/Extensions/Wireless360Controller.kext
fi

if [ -d /System/Library/Extensions/WirelessGamingReceiver.kext ]; then
   kextunload /System/Library/Extensions/WirelessGamingReceiver.kext
   /bin/rm -r /System/Library/Extensions/WirelessGamingReceiver.kext
fi

# Mavericks and later

if [ -d /Library/Extensions/360Controller.kext ]; then
   kextunload /Library/Extensions/360Controller.kext
   /bin/rm -r /Library/Extensions/360Controller.kext
fi

if [ -d /Library/Extensions/Wireless360Controller.kext ]; then
   kextunload /Library/Extensions/Wireless360Controller.kext
   /bin/rm -r /Library/Extensions/Wireless360Controller.kext
fi

if [ -d /Library/Extensions/WirelessGamingReceiver.kext ]; then
   kextunload /Library/Extensions/WirelessGamingReceiver.kext
   /bin/rm -r /Library/Extensions/WirelessGamingReceiver.kext
fi

exit 0
