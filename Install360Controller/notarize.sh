#!/bin/sh
# TODO(Drew): Update README to note changes to DeveloperSettings file

DEV_EMAIL=`echo | grep DEVELOPER_EMAIL ../DeveloperSettings.xcconfig`
NOTARIZATION_PASSWORD=`echo | grep NOTARIZATION_PASSWORD ../DeveloperSettings.xcconfig`
DMG_NAME=`echo | ls | grep '360ControllerInstall.*.dmg'`

AUTHENTICATION="--username \"${DEV_EMAIL//\DEVELOPER_EMAIL = }\" --password \"${NOTARIZATION_PASSWORD//\NOTARIZATION_PASSWORD = }\""

xcrun altool --notarize-app --primary-bundle-id "com.mice.driver" ${AUTHENTICATION} --file ${DMG_NAME}
xcrun stapler staple ${DMG_NAME}
xcrun stapler validate ${DMG_NAME}
