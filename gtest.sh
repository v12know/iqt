#!/bin/bash

# CONDA_HOME=~/anaconda3
# source $CONDA_HOME/bin/activate iqt

#BASE_PATH=$(cd `dirname $0`; pwd)
cd `dirname $0`
PROGRAM_NAME=./bin/gtest_bin
LD_LIBRARY_PATH=./lib

$PROGRAM_NAME
