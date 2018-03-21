#!/bin/bash

UVDIR="libuv"
COMMIT="c5afc37e2a8a70d8ab0da8dac10b77ba78c0488c"

if [ ! -d ${UVDIR} ]; then
    echo "Installing libuv..."
    git clone git@github.com:libuv/libuv.git ${UVDIR}
    cd ${UVDIR}
    git checkout ${COMMIT}
    ./autogen.sh
    ./configure
    make
    make install
fi;

if [ ! -f json.c ]; then
    echo "Downloading json-parser..."
    curl https://raw.githubusercontent.com/udp/json-parser/master/json.c > json.c
    curl https://raw.githubusercontent.com/udp/json-parser/master/json.h > json.h
fi;
