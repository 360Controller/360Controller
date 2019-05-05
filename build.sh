#!/bin/bash

DEV_NAME=`echo | grep DEVELOPER_NAME DeveloperSettings.xcconfig`
DEV_TEAM=`echo | grep DEVELOPMENT_TEAM DeveloperSettings.xcconfig`
CERT_ID="${DEV_NAME//\DEVELOPER_NAME = } (${DEV_TEAM//\DEVELOPMENT_TEAM = })"
VERSIONS=( 10.11 10.12 10.13 10.14 )

mkdir -p builds

for version in "${VERSIONS[@]}"
do
	xcrun xcodebuild -configuration Release -target "Whole Driver" -xcconfig "DeveloperSettings.xcconfig" MACOSX_DEPLOYMENT_TARGET=$version SDKROOT=macosx$version OTHER_CODE_SIGN_FLAGS="--timestamp --options=runtime"
	if [ $? -ne 0 ]
		then
			echo "******** BUILD FAILED ********"
			exit 1
	fi
	mv build/ builds/$version
done

pushd Install360Controller
packagesbuild -v Install360Controller.pkgproj --identity "Developer ID Installer: ""${CERT_ID}"
if [ $? -ne 0 ]
	then
		echo "******** INSTALLER BUILD FAILED ********"
		exit 1
fi
popd

for version in "${VERSIONS[@]}"
do
	echo "** File contents - $version **"
	xcrun lipo -info builds/$version/Release/360Controller.kext/Contents/MacOS/360Controller
	xcrun lipo -info builds/$version/Release/360Controller.kext/Contents/PlugIns/Feedback360.plugin/Contents/MacOS/Feedback360
	xcrun lipo -info builds/$version/Release/360Daemon.app/Contents/MacOS/360Daemon
	xcrun lipo -info builds/$version/Release/Pref360Control.prefPane/Contents/MacOS/Pref360Control
	xcrun lipo -info builds/$version/Release/Pref360Control.prefPane/Contents/Resources/DriverTool
	xcrun lipo -info builds/$version/Release/WirelessGamingReceiver.kext/Contents/MacOS/WirelessGamingReceiver
	xcrun lipo -info builds/$version/Release/Wireless360Controller.kext/Contents/MacOS/Wireless360Controller
	echo "** File signatures - $version **"
	xcrun spctl -a -v builds/$version/Release/360Controller.kext
	xcrun spctl -a -v builds/$version/Release/360Controller.kext/Contents/PlugIns/Feedback360.plugin
	xcrun spctl -a -v builds/$version/Release/360Daemon.app/Contents/MacOS/360Daemon
	xcrun spctl -a -v builds/$version/Release/Pref360Control.prefPane
	xcrun spctl -a -v builds/$version/Release/Pref360Control.prefPane/Contents/Resources/DriverTool
	xcrun spctl -a -v builds/$version/Release/WirelessGamingReceiver.kext
	xcrun spctl -a -v builds/$version/Release/Wireless360Controller.kext
done
echo "** Installer signature **"
xcrun spctl -a -v --type install Install360Controller/build/Install360Controller.pkg
echo "*** DONE ***"
