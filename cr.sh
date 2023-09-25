#!/bin/bash
# compile and run
make elf
cd ./build
./createimage --extended bootblock main # $@
cd ..
make run
