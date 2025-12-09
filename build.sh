#!/bin/bash
pushd build
cmake --build . --config RelWithDebInfo
popd

