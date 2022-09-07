#!/bin/bash
#
# a simple script that converts an input img file to ext3 format
# on a linux host machine
# 
# version: 0.1
#
# history:
#
#       2014-10-23  0.1 initial version
#
# usage:   $0  inImageFile [outImageFile]
# 
# 

_version=0.1
_prog=`basename $0`

function help()
{
    echo "$_prog version $_version"
    echo "usage: $_prog inImage [outImage]"
}

# Check input file
_IMG_I=$1
_IMG_O=$2
[ -z "$_IMG_I" ] && help && exit 1
[ ! -f "$_IMG_I" ] && echo "$_IMG_I doesn't exist!" && exit 1
file $_IMG_I | fgrep -q "filesystem data"
[ $? -ne 0 ] && echo "Is $_IMG_I a filesystem image?" && exit 1
[ -z "$_IMG_O" ] && _IMG_O=`basename $_IMG_I .img`.ext3.img

# preparing ext3 file image
echo "creating $_IMG_O..."
cp $_IMG_I $_IMG_O 
[ $? -ne 0 ] && echo "failed to generate $_IMG_O" && exit 2
yes|mkfs.ext3 $_IMG_O
[ $? -ne 0 ] && echo "failed to make ext3 filesystem on $_IMG_O" && exit 2 

# mounting images
_D_I=`mktemp -d`
_D_O=`mktemp -d`
trap "df | grep -e $_D_I'$' -e $_D_O'$' && sudo umount $_D_I $_D_O >/dev/null && rm -rf $_D_I $_D_O" EXIT

sudo mount $_IMG_I $_D_I 
[ $? -ne 0 ] && echo "Failed to mount $_IMG_I" && exit 1

sudo mount $_IMG_O $_D_O
[ $? -ne 0 ] && echo "Failed to mount $_IMG_O" && exit 2

# Copying files to ext3
( cd $_D_O && sudo cp -ar $_D_I/* .)

sync

# cleaning up
sudo umount $_D_I $_D_O && rm -rf $_D_I $_D_O
exit 0

