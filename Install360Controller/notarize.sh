#!/bin/sh
DEV_EMAIL=`echo | grep DEVELOPER_EMAIL ../DeveloperSettings.xcconfig`
NOTARIZATION_PASSWORD=`echo | grep NOTARIZATION_PASSWORD ../DeveloperSettings.xcconfig`
DMG_NAME=`echo | ls | grep '360ControllerInstall.*.dmg'`
USERNAME="${DEV_EMAIL//\DEVELOPER_EMAIL = }"
PASSWORD="${NOTARIZATION_PASSWORD//\NOTARIZATION_PASSWORD = }"

xcrun altool --notarize-app --primary-bundle-id "com.mice.driver" --username ${USERNAME} --password ${PASSWORD} --file ${DMG_NAME}

