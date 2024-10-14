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

# Function to remove version numbers from dylib file name
removeVersion() {
    sed -E 's/\.[0-9]+//g'
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

    # if directory "lib/libx264.a" doesn't exist (i.e. make install has not executed)
    if [ ! -e lib/libx264.a ]; then
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
        ./configure --prefix="$(pwd)" --extra-ldflags="-Wl,-rpath,@executable_path/dummy" --enable-gpl --disable-static --enable-shared --disable-programs --disable-doc --enable-libx264 --arch=arm64 --enable-cross-compile
    fi

    # Run make command
    assureExecute make

    # if directory "include/libavcodec" doesn't exist (i.e. make install has not executed)
    if [ ! -d include/libavcodec ]; then
        # install ffmpeg libraries to lib directory
        assureExecute make install

        ######### change ffmpeg shared libraries (.dylib) to rpath ########

        # Change to the library directory
        cd lib

        # for all *.dylib files in lib directory
        for lib_name in *.dylib; do
            lib_name_without_version=$(echo "$lib_name" | removeVersion)
            # change install path to itself
            install_name_tool -id "@rpath/$lib_name_without_version" "$lib_name"

            # loop for all the other *.dylib file names
            for other_lib_name in *.dylib; do
                other_lib_name_without_version=$(echo "$other_lib_name" | removeVersion)
                # change install path on this library to other_lib_name to rpath
                install_name_tool -change "$(pwd)/$other_lib_name" "@rpath/$other_lib_name_without_version" "$lib_name"
            done
        done

        # for all .*\.dylib or .*\..*\.dylib alias files in lib directory
        for alias_file_relpath in $(find . -type l); do
            # remove it
            rm $alias_file_relpath
        done

        # for all real dylib files
        for lib_name_with_version in *.dylib; do
            # remove dummy rpath
            install_name_tool -delete_rpath "@executable_path/dummy" "$lib_name_with_version"

            # rename it to a name that doesn't include the version number
            mv "$lib_name_with_version" "$(echo $lib_name_with_version | removeVersion)"
        done
    fi
)
