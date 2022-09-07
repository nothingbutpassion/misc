#!/bin/bash

set -e

script_dir=$(dirname `readlink -f $0`)

CROSS_BUILD=0
RELEASE_BUILD=0

# --- useage ---
function usage()
{
cat <<- EOT

    Usage: $0 [options] [--]

    Options:
        -c|cross         Cross Build
        -r|release       Release Build
        -h|help          Display this message

    Example:
        ./build.sh -c
EOT
}
# --- end of function ---

#---------------------------------#
#  Handle command line arguments  #
#---------------------------------#
while getopts "crh" opt
do
    case $opt in

        c|cross)
            CROSS_BUILD=1
            ;;
        
        r|release)
            RELEASE_BUILD=1
            ;;

        h|help)
            usage
            exit 0
            ;;

        \?)
            usage
            exit 1
            ;;

    esac    # --- end of case ---
done

#pre build
if [[ -f $script_dir/prebuild.sh ]]; then
    echo "===== start prebuild ====="
    if [[ $RELEASE_BUILD -eq 1 ]]; then
        if [[ $CROSS_BUILD -eq 1 ]]; then
            bash $script_dir/prebuild.sh -r -c
        else
            bash $script_dir/prebuild.sh -r
        fi
    else
        if [[ $CROSS_BUILD -eq 1 ]]; then
            bash $script_dir/prebuild.sh -c
        else
            bash $script_dir/prebuild.sh
        fi
    fi
    echo "===== end prebuild ====="
fi

if [[ $RELEASE_BUILD -eq 1 ]]; then
    export COMPILE_MODE="release"
else
    export COMPILE_MODE="debug"
fi

if [[ $CROSS_BUILD -eq 0 ]]; then
    #build x64
    echo "WARNING! Can only build for ARM target!" 
    exit 1
    #rm -rf $script_dir/build $script_dir/build_ut $script_dir/dist/x64
    #mkdir -p $script_dir/build_ut
    #cd $script_dir/build_ut
    #cmake ../unit_test
    #make
else
    #cross build
    if [[ `ls $HOME/adksetup/adk_env.sh 2>/dev/null | wc -l ` -ne 1 ]]; then
        echo -e "ERROR! Can NOT find the $HOME/adksetup/adk_env.sh \nPlease check your environment!"
        exit 1
    fi
    
    source $HOME/adksetup/adk_env.sh
    rm -rf $script_dir/build $script_dir/dist/arm
    mkdir -p $script_dir/build
    cd $script_dir/build
    cmake ../src
    make
fi

#post build
if [[ -f $script_dir/postbuild.sh ]]; then
    echo "===== start postbuild ====="
    if [[ $RELEASE_BUILD -eq 1 ]]; then
        if [[ $CROSS_BUILD -eq 1 ]]; then
            bash $script_dir/postbuild.sh -r -c
        else
            bash $script_dir/postbuild.sh -r
        fi
    else
        if [[ $CROSS_BUILD -eq 1 ]]; then
            bash $script_dir/postbuild.sh -c
        else
            bash $script_dir/postbuild.sh
        fi
    fi
    echo "===== end postbuild ====="
fi
