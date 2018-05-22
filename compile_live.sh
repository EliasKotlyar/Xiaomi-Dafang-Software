#!/usr/bin/env bash
cd live
./genMakefiles dafang
make clean
make -j4

