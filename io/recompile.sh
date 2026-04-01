#!/bin/bash

cd out
make clean
bear -- make
mv compile_commands.json ..
make install
