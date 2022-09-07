#!/bin/bash
#
# A simple script to collect adb log, need cygwin to run on Windows.
# Copyright 2014 Ygomi LLC, All rights reserved.
#
# Version     0.0.1 by yuhao
#

__version=0.0.1
__program=`basename $0` 


show_help() {
	echo "Usage: $__program [options] <device-ip> "
    echo "  options:"
	echo "    -n <name> : specify the log file name (default: test)"
    echo "    -h        : show this help message"
    echo "    -v        : show version info"
    echo "  device-ip: specify the android device ipv4 address to connect"
    echo "Examples: "
    echo "  $__program 10.69.2.160"
    echo "  $__program -n mytest 10.69.2.160"
}


start_connect() { 
    _deviceIP=$1

    # Disconnect devices
    echo "--> Disconnect adb connection..."
    adb disconnect 
    _deviceCount=`adb devices | grep -v ^List | grep device |  wc -l`
    if [ $_deviceCount -gt 0 ]; then
        echo "Please disconnec or close the following devices before your execting this script."
        adb devices | grep -v ^List
        exit 1
    fi

    # Connect the specified device
    echo "--> Try to adb connect $_deviceIP..."
    adb connect $_deviceIP
    _deviceCount=`adb devices | grep -v ^List |  grep -F "$_deviceIP" | wc -l`
    while [ $_deviceCount != 1 ]; do
        echo "Connect to $_deviceIP failed, sleep 1 seconds and try again...(Press Ctrl+C to abort)"
        sleep 1
        adb connect $_deviceIP
        _deviceCount=`adb devices | grep -v ^List |  grep -F "$_deviceIP" | wc -l`
    done   
}


start_logcat() {
    _logFile=$1
    
    # Start logcat
    echo "--> Starting logcat"
    adb logcat -v threadtime >$_logFile  2>&1 &

    # Wait for log file
    while [ ! -e "$_logFile" ]; do
        echo "--> Waiting log file: $_logFile"
        sleep 1
    done  
}


kill_adb() {
    for pid in `ps -ef  | grep -v "grep" | grep "adb" | awk '{print $2}'`
    do
        if [ "$pid" != "$$" ]; then
            echo "Killing pid = $pid"
            kill $pid
        fi
    done
}

file_size() {
    stat -c"%s" $1
    #_logFile=$1
    #_cygwin=`uname -o | tr [A-Z] [a-z] | grep cygwin` 
    #if [ "$_cygwin" != "" ]; then
    #    _fileSize=`ls -l $_logFile | awk '{print $6}'`
    #else
    #    _fileSize=`ls -l $_logFile | awk '{print $5}'`
    #fi
    #echo $_fileSize
}

__name=test
while getopts ":hvn:" opt; do
	case $opt in
    h) 
		show_help
		exit 0 ;;
    v) 
		echo "Current version of $__program is $__version"
		exit 0 ;;
	n) 
        __name=${OPTARG} ;;
	\?) 
		echo "Invalid option: -$OPTARG"
		exit 1 ;;
	:)
		echo "Option -$OPTARG requires an argument"
		exit 1 ;;
	*)
		show_help
		exit 1 ;;
	esac
done
shift $(($OPTIND -1))

[ ! $# -eq 1 ] && show_help &&  exit 1
__isIpv4=$(echo $1 | awk -F '.' '$1 < 255 && $1 >= 0 && $2 < 255 && $2 >= 0 && $3 < 255 && $3 >= 0 && $4 < 255 && $4 >= 0 {print 1}')
[ "$__isIpv4" != "1" ] && echo "$1 is invalid ipv4 address" && exit 1
__deviceIp=$1

trap "kill_adb; exit 1" SIGINT

start_connect $__deviceIp
__logFile=`date +"$__name.%Y%m%d.%H%M%S.txt"`
start_logcat $__logFile
__logSize=`file_size  $__logFile`

while :
do
    echo "--> Sleeping for 10 seconds...(Press Ctrl+C to abort)"
    sleep 10

    __size=`file_size  $__logFile`
    echo "--> The size of $__logFile is $__size bytes."
    if [ $__size -le $__logSize ]; then
        # This may be a network problem, we assume device has been closed or restarted.
        echo "--> The size of $__logFile hasn't been changed. restarting..."
        kill_adb
        start_connect $__deviceIp
        __logFile=`date +"$__name.%Y%m%d.%H%M%S.txt"`
        start_logcat $__logFile
    fi
    __logSize=`file_size  $__logFile`
done




