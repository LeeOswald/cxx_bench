#!/bin/bash

if [ ! -d "build" ]; then
    mkdir build
fi

pushd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
popd

