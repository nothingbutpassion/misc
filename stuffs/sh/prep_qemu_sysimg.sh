#!/bin/bash
#
# a simple tool to prepare system.img proper for running with main stream
# Linux kernel.
#
# Revisions:

#   0.1 Yanfeng initial version
#
# Usage:
#
#    $0 input_yaffs_img_file
#
#    result is infile.ext4 image file along with input file
#
# 

_ver=0.1

[ $# -lt 1 ] && echo please specifiy input yaffs image file name && exit 1

_inFile=`readlink -f $1`

[ ! -s $_inFile ] && echo $_inFile does not exist! && exit 2

file $_inFile | fgrep -q "VMS Alpha" 
[ $? -ne 0 ] && echo $_inFile is not YAFFS image? && exit 2


_inSize=$(stat -c%s "$_inFile")
_outSize=`echo "$_inSize * 1.3 / 1024 / 1024 + 1 "|bc`
_outFile=${_inFile}.ext4

echo Input file size is ${_inSize}B
echo Preparing ${_outSize}MB image file $_outFile...
dd if=/dev/zero of=$_outFile bs=1M count=$_outSize 
[ $? -ne 0 ] && exit 3
yes|mkfs.ext4 $_outFile
[ $? -ne 0 ] && exit 3

echo Copying YAFFS image to new image...
_mntPoint=`mktemp -d`
sudo mount -o loop $_outFile $_mntPoint
[ $? -ne 0 ] && exit 3

trap "[ -d $_mntPoint ] && sudo umount $_mntPoint && rmdir $_mntPoint" EXIT INT TERM

(cd $_mntPoint; sudo `which unyaffs` $_inFile; sync)

if [ -r $_mntPoint/lib ]; then 
    echo Cleaning up unnecessay files...
    _files2Clean="hw/gralloc.goldfish.so egl/libEGL_emulation.so \
        egl/libGLESv1_CM_emulation.so egl/libGLESv2_emulation.so \
        libGLESv1_enc.so libGLESv2_enc.so"
    (cd $_mntPoint/lib && sudo rm -f $_files2Clean)
    echo Updating egl.cfg...
    (cd $_mntPoint/lib/egl && sudo su -c "echo 0 0 android>egl.cfg" )
fi

sudo umount $_mntPoint 
rmdir $_mntPoint

echo done!

