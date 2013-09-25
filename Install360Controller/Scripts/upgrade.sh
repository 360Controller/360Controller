#!/bin/sh

# Clear out daemon

if [ -d /Library/StartupItems/360ControlDaemon ]; then
   /bin/rm -r /Library/StartupItems/360ControlDaemon
fi

if [ -f /Library/LaunchDaemons/com.mice.360Daemon.plist ]; then
   launchctl stop com.mice.360Daemon
   /bin/rm /Library/LaunchDaemons/com.mice.360Daemon.plist
fi

if [ -d /Library/Application\ Support/MICE/360Daemon ]; then
   /bin/rm -r /Library/Application\ Support/MICE/360Daemon
fi

# Remove preference pane

if [ -d /Library/PreferencePanes/Pref360Control.prefPane ]; then
   /bin/rm -r /Library/PreferencePanes/Pref360Control.prefPane
fi

# Remove drivers

if [ -d /System/Library/Extensions/360Controller.kext ]; then
   /bin/rm -r /System/Library/Extensions/360Controller.kext
fi

if [ -d /System/Library/Extensions/Wireless360Controller.kext ]; then
   /bin/rm -r /System/Library/Extensions/Wireless360Controller.kext
fi

if [ -d /System/Library/Extensions/WirelessGamingReceiver.kext ]; then
   /bin/rm -r /System/Library/Extensions/WirelessGamingReceiver.kext
fi

exit 0
