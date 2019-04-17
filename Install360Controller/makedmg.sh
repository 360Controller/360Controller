#!/bin/sh

hdiutil create -volname 360ControllerInstall -srcfolder ./build -ov -fs HFS+ -format UDZO 360ControllerInstall.dmg
