#!/bin/sh
osascript -e "do shell script \"

### Stop the daemon ###

echo \\\"Check to see if the 360Daemon is installed.\\\"
if [ -f /Library/LaunchDaemons/com.mice.360Daemon.plist ]; then
	echo \\\"Unloading 360Daemon.\\\"
	/bin/launchctl stop com.mice.360Daemon 2> /dev/null
	/bin/launchctl unload /Library/LaunchDaemons/com.mice.360Daemon.plist 2> /dev/null
fi



### Unload all loaded drivers ###

driverIdentifiers=(
	\\\"com.mice.driver.Xbox360Controller\\\"
	\\\"com.mice.driver.WirelessGamingReceiver\\\"
	\\\"com.mice.driver.Wireless360Controller\\\"
	\\\"com.mice.driver.WirelessOneController\\\"
)

for index in \\\"${!driverIdentifiers[@]}\\\"; do

	echo \\\"Checking to see if ${driverIdentifiers[$index]} is loaded.\\\"
	/usr/sbin/kextstat | grep \\\"${driverIdentifiers[$index]}\\\" 2> /dev/null

	if [ $? -eq 0 ]; then
		echo \\\"Unloading ${driverIdentifiers[$index]}.\\\"
		/sbin/kextunload -q -b "${driverIdentifiers[$index]}" 2> /dev/null
	else
		continue
	fi

	if [ $? -ne 0 ]; then
		echo Failed to unload one of the drivers that make up 360Controller. \
		Please ensure that all devices that use the driver are unplugged from your computer, \
		and then please reboot your system before attempting to uninstall again.
		exit 1
	fi
done



### Delete the products of the driver ###

deletionItems=(
	\\\"/Library/StartupItems/360ControlDaemon\\\"
	\\\"/Library/Application Support/MICE/360Daemon\\\"
	\\\"/Library/Application Support/MICE/360Daemon.app\\\"
	\\\"/Library/PreferencePanes/Pref360Control.prefPane\\\"
	\\\"/Library/LaunchDaemons/com.mice.360Daemon.plist\\\"
	\\\"/System/Library/Extensions/360Controller.kext\\\"
	\\\"/System/Library/Extensions/Wireless360Controller.kext\\\"
	\\\"/System/Library/Extensions/WirelessGamingReceiver.kext\\\"
	\\\"/Library/Extensions/360Controller.kext\\\"
	\\\"/Library/Extensions/Wireless360Controller.kext\\\"
	\\\"/Library/Extensions/WirelessGamingReceiver.kext\\\"
	\\\"/Library/Extensions/XboxOneBluetooth.kext\\\"
)

for index in \\\"${!deletionItems[@]}\\\"; do

	echo \\\"Deleting ${deletionItems[$index]}.\\\"

	if [ -d \\\"${deletionItems[$index]}\\\" ]; then
		/bin/rm -r \\\"${deletionItems[$index]}\\\" 2> /dev/null
	else
		continue
	fi

	if [ $? -eq 0 ]; then
		echo Failed to delete one of the products of 360Controller. \
		Please attempt to manually delete the file at ${deletionItems[$index]}
		exit 1
	fi
done


echo \\\"Uninstalled 360Controller successfully.\\\"

\" with administrator privileges"
