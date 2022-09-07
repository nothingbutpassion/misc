#!/bin/sh


while :; do
    read x;
    echo $x;	
    if [ "$x" = "stop"  ]; then
        break;		
    fi	
done
