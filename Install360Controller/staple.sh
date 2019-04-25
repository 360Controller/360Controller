#!/bin/sh
DMG_NAME=`echo | ls | grep '360ControllerInstall.*.dmg'`
xcrun stapler staple ${DMG_NAME}
xcrun stapler validate ${DMG_NAME}

