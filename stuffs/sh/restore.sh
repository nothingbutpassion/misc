#!/bin/bash

if [[ ! $1 ]]; then
    echo -e "Usage:\n\n\t$0 'file name'"
    exit 1
fi

f=$(ls 2>/dev/null -l /proc/*/fd/* | fgrep "$1 (deleted" | awk '{print $9}')

if [[ $f ]]; then
    echo "fd $f found..."
    cp -v "$f" "$1"
else
    echo >&2 "No fd found..."
    exit 2
fi
