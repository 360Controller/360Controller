#!/bin/sh

# Get the major and minor versions of the current OS
MAJOR_VER=$(sw_vers -productVersion | awk -F. '{ print $1; }')
MINOR_VER=$(sw_vers -productVersion | awk -F. '{ print $2; }')

# Just have to make sure that macoS didn't update to 11 and really mess things up
if [[ "${MAJOR_VER}" -ne 10 ]]; then
    echo "Major version is not 10. Cowardly failing out of script execution."
    exit 1
fi

# This is to fix a bug in Yosemite.
if [[ "${MINOR_VER}" -le 14 && "${MINOR_VER}" -ge 10 ]]; then
    /bin/ln -s /Library/Extensions/360Controller.kext /System/Library/Extensions/
fi

# Make the uninstall script in the preference pane executable
/bin/chmod ug+x /Library/PreferencePanes/Pref360Control.prefPane/Contents/Resources/upgrade.sh

/usr/sbin/kextcache -i /
/usr/bin/touch /Library/Extensions

/bin/launchctl load -w /Library/LaunchDaemons/com.mice.360Daemon.plist

exit 0
