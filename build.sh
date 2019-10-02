#!/bin/bash

DEV_NAME=`echo | grep DEVELOPER_NAME DeveloperSettings.xcconfig`
DEV_TEAM=`echo | grep DEVELOPMENT_TEAM DeveloperSettings.xcconfig`
CERT_ID="${DEV_NAME//\DEVELOPER_NAME = } (${DEV_TEAM//\DEVELOPMENT_TEAM = })"
mkdir -p build


xcrun xcodebuild -configuration Release -target "Whole Driver" -xcconfig "DeveloperSettings.xcconfig" MACOSX_DEPLOYMENT_TARGET=10.11 SDKROOT=macosx10.14 OTHER_CODE_SIGN_FLAGS="--timestamp --options=runtime"
if [ $? -ne 0 ]
	then
		echo "******** BUILD FAILED ********"
		exit 1
fi

pushd Install360Controller
packagesbuild -v Install360Controller.pkgproj --identity "Developer ID Installer: ""${CERT_ID}"
if [ $? -ne 0 ]
	then
		echo "******** INSTALLER BUILD FAILED ********"
		exit 1
fi
popd

echo "** File contents **"
xcrun lipo -info build/Release/360Controller.kext/Contents/MacOS/360Controller
xcrun lipo -info build/Release/360Controller.kext/Contents/PlugIns/Feedback360.plugin/Contents/MacOS/Feedback360
xcrun lipo -info build/Release/360Daemon.app/Contents/MacOS/360Daemon
xcrun lipo -info build/Release/Pref360Control.prefPane/Contents/MacOS/Pref360Control
xcrun lipo -info build/Release/Pref360Control.prefPane/Contents/Resources/DriverTool
xcrun lipo -info build/Release/WirelessGamingReceiver.kext/Contents/MacOS/WirelessGamingReceiver
xcrun lipo -info build/Release/Wireless360Controller.kext/Contents/MacOS/Wireless360Controller
xcrun lipo -info build/Release/WirelessOneController.kext/Contents/MacOS/WirelessOneController
echo "** File signatures **"
xcrun spctl -a -v build/Release/360Controller.kext
xcrun spctl -a -v build/Release/360Controller.kext/Contents/PlugIns/Feedback360.plugin
xcrun spctl -a -v build/Release/360Daemon.app/Contents/MacOS/360Daemon
xcrun spctl -a -v build/Release/Pref360Control.prefPane
xcrun spctl -a -v build/Release/Pref360Control.prefPane/Contents/Resources/DriverTool
xcrun spctl -a -v build/Release/WirelessGamingReceiver.kext
xcrun spctl -a -v build/Release/Wireless360Controller.kext
xcrun spctl -a -v build/Release/WirelessOneController.kext

echo "** Installer signature **"
xcrun spctl -a -v --type install Install360Controller/build/Install360Controller.pkg
echo "*** DONE ***"
