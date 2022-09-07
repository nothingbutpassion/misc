#!/bin/bash

this_dir=$(dirname $(readlink -f $0))
proj_dir=$(dirname $(dirname ${this_dir}))
module_name=vpu_codec
test_app_name=test_vpu_jpeg
test_src_files=(test_vpu_jpeg.cpp vpu_enc_jpeg.cpp vpu_dec_jpeg.cpp)
test_app_path=${proj_dir}/dist/x64/bin/${test_app_name}

test_out_dir=${proj_dir}/unit_test_output
gcov_out_dir=${test_out_dir}/coverage/${module_name}/${test_app_name}


#
# generate testing data
#
echo "Generating testing data ..."

mkdir -p ${test_out_dir}
rm -rf ${test_out_dir}/${module_name}-${test_app_name}-unittest.xml

${test_app_path} --gtest_output=xml:${test_out_dir}/${module_name}-${test_app_name}-unittest.xml

echo "Testing data is saved in ${test_out_dir}"
ls -l ${test_out_dir}/${module_name}-unittest.xml


#
# generate coverage data
#
echo "Generating coverage data ..."

mkdir -p ${gcov_out_dir}
rm -rf ${gcov_out_dir}/*

for s in ${test_src_files[@]}; do
	find ${proj_dir} -name $s.gcno -o -name $s.gcda | xargs -I {} cp -v {} ${gcov_out_dir}
done

old_dir=$(pwd)
cd ${gcov_out_dir}
gcov *.gcno
for s in ${test_src_files[@]}; do
	mv $s.gcov $s.gcov.filter
done
rm *.gcov
for s in ${test_src_files[@]}; do
	mv $s.gcov.filter $s.gcov
done
cd ${old_dir}

echo "Coverage data is saved in ${gcov_out_dir}"
ls -l ${gcov_out_dir}

