#!/bin/bash
pushd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
popd

