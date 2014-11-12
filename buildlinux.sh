#!/bin/bash
[ -d build ] || mkdir build
cd build
cmake .. -G "Unix Makefiles"
