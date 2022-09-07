#!/bin/sh

usage() { 
    echo "Usage: $0 [-d <gc-search-dir>] [-o <html-out-dir>]" 1>&2; exit 1;
}


gcdir=./
outdir=./gcov_htmls

while getopts ":d:o:" opt; do
    case "${opt}" in
        d) gcdir=${OPTARG} ;;
        o) outdir=${OPTARG} ;;
        *) usage ;;
    esac
done

mkdir -p ${outdir}
rm -rv ${outdir}/*
find $gcdir -name '*.gcno' -o -name '*.gcda' | xargs -I {}  cp -v {} ${outdir}/
lcov --directory ${outdir} --capture --output-file ${outdir}/gcov.dat
genhtml ${outdir}/gcov.dat -o ${outdir}








