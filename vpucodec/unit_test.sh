#!/bin/bash

cur_dir=$(pwd)/unit_test
cd ${cur_dir}
for dirlist in $(ls .)
do
    if [ -x ${dirlist}/run.sh ] ; then
        cd ${dirlist}
        echo "===================== Start to run unit test:$dirlist =================="
        ./run.sh
        cd ..
        echo "===================== $dirlist done. =================="
    fi
done


