#!/bin/bash

BUILD_DIR=build
BUILD_TYPE=DEBUG

[ -d ${BUILD_DIR} ] || mkdir ${BUILD_DIR}
pushd ${BUILD_DIR}
cmake \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_TOOLCHAIN_FILE=../../rpi3/rpidevenv/rpi3toolchain.cmake \
	../encoderController/

make -j4 #VERBOSE=1
popd
