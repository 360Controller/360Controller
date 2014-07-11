#!/bin/bash
mkdir -p build
zip -r build/360ControllerSource.zip * -x "build*"

xcrun xcodebuild -configuration Release -target "Whole Driver"
if [ $? -ne 0 ]
  then
    echo "******** BUILD FAILED ********"
    exit 1
fi

cd Install360Controller
packagesbuild -v Install360Controller.pkgproj
mv build 360ControllerInstall
hdiutil create -srcfolder 360ControllerInstall -format UDZO ../build/360ControllerInstall.dmg
mv 360ControllerInstall build
cd ..
echo "** File contents **"
xcrun lipo -info build/Release/360Controller.kext/Contents/MacOS/360Controller
xcrun lipo -info build/Release/360Controller.kext/Contents/PlugIns/Feedback360.plugin/Contents/MacOS/Feedback360
xcrun lipo -info build/Release/360Daemon.app/Contents/MacOS/360Daemon
xcrun lipo -info build/Release/Pref360Control.prefPane/Contents/MacOS/Pref360Control
xcrun lipo -info build/Release/Pref360Control.prefPane/Contents/Resources/DriverTool
xcrun lipo -info build/Release/WirelessGamingReceiver.kext/Contents/MacOS/WirelessGamingReceiver
xcrun lipo -info build/Release/Wireless360Controller.kext/Contents/MacOS/Wireless360Controller
echo "** File signatures **"
xcrun spctl -a -v build/Release/360Controller.kext
xcrun spctl -a -v build/Release/360Controller.kext/Contents/PlugIns/Feedback360.plugin
xcrun spctl -a -v build/Release/360Daemon.app/Contents/MacOS/360Daemon
xcrun spctl -a -v build/Release/Pref360Control.prefPane
xcrun spctl -a -v build/Release/Pref360Control.prefPane/Contents/Resources/DriverTool
xcrun spctl -a -v build/Release/WirelessGamingReceiver.kext
xcrun spctl -a -v build/Release/Wireless360Controller.kext
xcrun spctl -a -v --type install Install360Controller/build/Install360Controller.pkg
echo "*** DONE ***"
