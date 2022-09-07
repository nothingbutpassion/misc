#!/bin/bash
set -v
APK_NAME=$1
PACKAGE_NAME=$2
adb uninstall $PACKAGE_NAME
adb install -r $APK_NAME
mid=`adb shell ime list -a | grep "^$PACKAGE_NAME" | sed s/://`
if [ -n $ime_id ]; then
	adb shell ime set "$mid"
fi
