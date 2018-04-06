#!/bin/bash

WORKDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
UVDIR="libuv"
UVCOMMIT="c5afc37e2a8a70d8ab0da8dac10b77ba78c0488c"
CHECKDIR="check"
CHECKCOMMIT="65e8c5e36f2841be16c4c96bae7a7bc171ff7d67"
JSONDIR="json"
TPDIR="thpool"

if [ ! -d ${UVDIR} ]; then
    echo "Installing libuv..."
    git clone git@github.com:libuv/libuv.git ${UVDIR}
    cd ${UVDIR}
    git checkout ${UVCOMMIT}
    ./autogen.sh
    ./configure
    make
    make install
    cd ${WORKDIR}
fi;

if [ ! -d ${CHECKDIR} ]; then
    echo "Installing check..."
    git clone git@github.com:libcheck/check.git ${CHECKDIR}
    cd ${CHECKDIR}
    git checkout ${CHECKCOMMIT}
    mkdir build
    cd build
    cmake ../
    make
    cd ${WORKDIR}
fi;

if [ ! -d ${JSONDIR} ]; then
    echo "Downloading json lib..."
    mkdir ${JSONDIR}
    curl https://raw.githubusercontent.com/udp/json-parser/master/json.c > \
        ${JSONDIR}/json.c
    curl https://raw.githubusercontent.com/udp/json-parser/master/json.h > \
        ${JSONDIR}/json.h
    curl https://raw.githubusercontent.com/udp/json-builder/master/json-builder.c > \
        ${JSONDIR}/json-builder.c
    curl https://raw.githubusercontent.com/udp/json-builder/master/json-builder.h > \
        ${JSONDIR}/json-builder.h
fi;

if [ ! -d ${TPDIR} ]; then
    echo "Downloading thread pool lib..."
    mkdir ${TPDIR}
    curl https://raw.githubusercontent.com/Pithikos/C-Thread-Pool/master/thpool.h > \
        ${TPDIR}/thpool.h
    curl https://raw.githubusercontent.com/Pithikos/C-Thread-Pool/master/thpool.c > \
        ${TPDIR}/thpool.c
fi;
