#!/bin/bash

if [ -z "$PROJECT_ROOT" ]
then
    echo "PROJECT_ROOT is not set"
    exit 1
fi

# Generate build script
cd $PROJECT_ROOT && \
if [ ! -d build ]; then
    mkdir build
fi
cd build && \
cmake ../ -DCMAKE_INSTALL_PREFIX=$PROJECT_ROOT && \

# Build and install the program
make -j4 && \
make install && \

# Run the program
cd ../bin && \
./comp_graph_project