#!/bin/bash
mkdir -p build
zip -r build/360ControllerSource.zip * -x "build*"

cd Feedback360
xcrun xcodebuild -configuration Deployment
if [ $? -ne 0 ]
  then
    echo "******** BUILD FAILED ********"
    exit 1
fi
cd ../DriverTool
xcrun xcodebuild -configuration Release
if [ $? -ne 0 ]
  then
    echo "******** BUILD FAILED ********"
    exit 1
fi
cd ../360Controller
xcrun xcodebuild -configuration Deployment
if [ $? -ne 0 ]
  then
    echo "******** BUILD FAILED ********"
    exit 1
fi
cd ../360Daemon
xcrun xcodebuild -configuration Release
if [ $? -ne 0 ]
  then
    echo "******** BUILD FAILED ********"
    exit 1
fi
cd ../WirelessGamingReceiver
xcrun xcodebuild -configuration Release
if [ $? -ne 0 ]
  then
    echo "******** BUILD FAILED ********"
    exit 1
fi
cd ../Wireless360Controller
xcrun xcodebuild -configuration Release
if [ $? -ne 0 ]
  then
    echo "******** BUILD FAILED ********"
    exit 1
fi
cd ../Pref360Control
xcrun xcodebuild -configuration Deployment 
if [ $? -ne 0 ]
  then
    echo "******** BUILD FAILED ********"
    exit 1
fi
cd ../Install360Controller
packagesbuild -v Install360Controller.pkgproj
mv build 360ControllerInstall
hdiutil create -srcfolder 360ControllerInstall -format UDZO ../build/360ControllerInstall.dmg
mv 360ControllerInstall build
cd ..
echo "** File contents **"
xcrun lipo -info 360Controller/build/Deployment/360Controller.kext/Contents/MacOS/360Controller
xcrun lipo -info 360Controller/build/Deployment/360Controller.kext/Contents/PlugIns/Feedback360.plugin/Contents/MacOS/Feedback360
xcrun lipo -info 360Daemon/build/Release/360Daemon
xcrun lipo -info Pref360Control/build/Deployment/Pref360Control.prefPane/Contents/MacOS/Pref360Control
xcrun lipo -info Pref360Control/build/Deployment/Pref360Control.prefPane/Contents/Resources/DriverTool
xcrun lipo -info WirelessGamingReceiver/build/Release/WirelessGamingReceiver.kext/Contents/MacOS/WirelessGamingReceiver
xcrun lipo -info Wireless360Controller/build/Release/Wireless360Controller.kext/Contents/MacOS/Wireless360Controller
echo "** File signatures **"
xcrun spctl -a -v 360Controller/build/Deployment/360Controller.kext
xcrun spctl -a -v 360Controller/build/Deployment/360Controller.kext/Contents/PlugIns/Feedback360.plugin
xcrun spctl -a -v 360Daemon/build/Release/360Daemon
xcrun spctl -a -v Pref360Control/build/Deployment/Pref360Control.prefPane
xcrun spctl -a -v Pref360Control/build/Deployment/Pref360Control.prefPane/Contents/Resources/DriverTool
xcrun spctl -a -v WirelessGamingReceiver/build/Release/WirelessGamingReceiver.kext
xcrun spctl -a -v Wireless360Controller/build/Release/Wireless360Controller.kext
xcrun spctl -a -v --type install Install360Controller/build/Install360Controller.pkg
echo "*** DONE ***"
