#!/bin/bash

[[ $# -lt 1 ]] && echo "Please specify the last part of the ip" && exit -1;
[[ ! -d /dev/sdb2 ]] && echo "/dev/sdb2 doesn't exist" && exit -1;
set -x

# get ip and gateway value
LAST_IP=$1 
if [[ $LAST_IP == "55" ]]; then
    IP_ADDRESS="192.168.3.$LAST_IP"
    GATEWAY="192.168.3.254"
else
    IP_ADDRESS="10.69.2.$LAST_IP"
    GATEWAY="10.69.2.254"
fi

# mount sdb2
TMP_DIR=`mktemp -d`
mkdir -p $TMP_DIR/sdb2
sudo mount /dev/sdb2 $TMP_DIR/sdb2

# config ip and gateway
sudo sed -i  "s/IP_ADDR=.*/IP_ADDR=\"$IP_ADDRESS\"/" $TMP_DIR/sdb2/etc/udev/rules.d/ifup_debug.sh
sudo sed -i  "s/GATEWAY=.*/GATEWAY=\"$GATEWAY\"/" $TMP_DIR/sdb2/etc/udev/rules.d/ifup_debug.sh
sudo sed -i  "s/\# \/sbin\/route/\/sbin\/route/" $TMP_DIR/sdb2/etc/udev/rules.d/ifup_debug.sh
if [[ $LAST_IP == "55" ]]; then
    sudo sed -i  "s/10.0.0.0\/8/192.168.3.0\/24/" $TMP_DIR/sdb2/etc/sysconfig/iptables
    sudo sed -i  "s/\/sbin\/route/\# \/sbin\/route/" $TMP_DIR/sdb2/etc/udev/rules.d/ifup_debug.sh
else
    sudo sed -i  "s/192.168.3.0\/24/10.0.0.0\/8/" $TMP_DIR/sdb2/etc/sysconfig/iptables
fi

# umount sdb2
sudo umount $TMP_DIR/sdb2
rm -rf $TMP_DIR
sync

set +x
