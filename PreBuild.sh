#!/bin/bash

set -e

# Set homebrew path explicitly
export PATH="$PATH:/opt/homebrew/bin:/opt/homebrew/sbin"

# Function to check if a command exists
assureHasCommand() {
    if ! command -v "$1" &> /dev/null; then
        echo "ERROR: $1 is not recognized as a command." 1>&2
        echo "PATH = $PATH"
        exit 9009
    fi
}

# Function to execute a command and check its result
assureExecute() {
    "$@"
    if [ $? -ne 0 ]; then
        echo "ERROR: Command \"$*\" failed." 1>&2
        echo "PATH = $PATH"
        exit $?
    fi
}

# Main script
(
    # Ensure make command exists
    assureHasCommand make


    ######### Make x264 #########

    # Change to the target directory
    cd "$(dirname "$0")/Source/ThirdParty/x264/"

    # if config.h doesn't exist (i.e. configure has not executed)
    if [ ! -e config.h ]; then
        # invoke configure
        ./configure --prefix="$(pwd)" --enable-static --enable-lto --enable-pic --enable-strip
    fi

    # Run make command
    assureExecute make

    # if directory "lib" doesn't exist (i.e. make install has not executed)
    if [ ! -d lib ]; then
        assureExecute make install
    fi

    
    ######### Make ffmpeg ########

    # Change to the target directory
    cd "$(dirname "$0")/Source/ThirdParty/FFmpeg/ffmpeg/"

    # set PKG_CONFIG_PATH
    export PKG_CONFIG_PATH="../../x264/lib/pkgconfig"

    # if config.h doesn't exist (i.e. configure has not executed)
    if [ ! -e config.h ]; then
        # invoke configure
        ./configure --prefix="$(pwd)" --enable-rpath --enable-gpl --disable-static --enable-shared --disable-programs --disable-doc --enable-libx264 --arch=arm64 --enable-cross-compile
    fi

    # Run make command
    assureExecute make

    # if directory "lib" doesn't exist (i.e. make install has not executed)
    if [ ! -d lib ]; then
        assureExecute make install
    fi


    ######### change ffmpeg shared libraries (.dylib) to rpath ########

    # Change to the library directory
    cd lib

    # for all *.dylib files in lib directory
    for lib_name in *.dylib; do
        # change install path to itself
        install_name_tool -id "@rpath/$lib_name" "$lib_name"

        # loop for all the other *.dylib file names
        for other_lib_name in *.dylib; do
            # change install path on this library to other_lib_name to rpath
            install_name_tool -change "$(pwd)/$other_lib_name" "@rpath/$other_lib_name" "$lib_name"
        done
    done
)
