#!/bin/bash

[ $# -lt 1 ] && echo "Please specify img dir as the first argument." && exit -1; 
[ ! -d $1 ] && echo "$1 is not a dir" && exit -1;

set -x

#IMG_DIR=~/ssh-247-home/vehiclemw/out/target/product/generic_x86
IMG_DIR=${1%/}
TMP_DIR=`mktemp -d`

mkdir -p $TMP_DIR/system
mkdir -p $TMP_DIR/sdb5
cp $IMG_DIR/ramdisk.img $TMP_DIR/
cp $IMG_DIR/system.img $TMP_DIR/

# Delete all files in sdb5
sudo mount /dev/sdb5 $TMP_DIR/sdb5
sudo rm -rf $TMP_DIR/sdb5/*

# Copy ramdisk.img to sdb5
sudo cp $TMP_DIR/ramdisk.img $TMP_DIR/sdb5/

# Mount system.img to sdb5
sudo mount -o loop $TMP_DIR/system.img $TMP_DIR/system
sudo mkdir -p $TMP_DIR/sdb5/system
sudo cp -ar $TMP_DIR/system/*  $TMP_DIR/sdb5/system/
sudo umount $TMP_DIR/system

# Build data dir in sdb5
sudo mkdir -p $TMP_DIR/sdb5/data
sudo chown -R 1000:1000 $TMP_DIR/sdb5/data
sudo umount $TMP_DIR/sdb5
sync

# Clean generated files
rm -rf $TMP_DIR
sync

set +x
