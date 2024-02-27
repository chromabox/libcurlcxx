#!/bin/bash

check_and_create_dir()
{
    if [ -n "$1" ]; then
        mkdir -p $1
    fi
}

build_debug()
{
    check_and_create_dir build
    cd build

    cmake -DCMAKE_BUILD_TYPE=Debug ..
    cmake --build .

    cd ../
}

build_debug_with_sample()
{
    check_and_create_dir build
    cd build

    cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SAMPLE=ON ..
    cmake --build .

    cd ../
}


build_release()
{
    check_and_create_dir build_release
    cd build_release

    cmake -DCMAKE_BUILD_TYPE=Release ..
    cmake --build .

    cd ../
}

build_release_with_sample()
{
    check_and_create_dir build_release
    cd build_release

    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SAMPLE=ON ..
    cmake --build .

    cd ../
}


clean_build()
{
    rm -rf  build
    rm -rf  build_release
}

check_and_create_dir build
check_and_create_dir build_release

# オプション指定の振り分け
if [ "${1}" = "--clean" ]; then
    clean_build
    exit 0
elif [ "${1}" = "--debug" ]; then
    build_debug
    echo "done."
    exit 0
elif [ "${1}" = "--release" ]; then
    build_release
    echo "done."
    exit 0
elif [ "${1}" = "--debugsample" ]; then
    build_debug_with_sample
    echo "done."
    exit 0
elif [ "${1}" = "--releasesample" ]; then
    build_release_with_sample
    echo "done."
    exit 0
fi

build_debug

echo "done."
