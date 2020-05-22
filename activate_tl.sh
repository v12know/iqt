#!/bin/bash

CONDA_HOME=/usr/local/anaconda3
source $CONDA_HOME/bin/activate iqt

#BASE_PATH=$(cd `dirname $0`; pwd)
cd `dirname $0`
export LD_LIBRARY_PATH=./lib

PROGRAM_NAME=./main.py
CONFIG_FILE=./config/time_line_dev.json

python $PROGRAM_NAME $CONFIG_FILE --script $BASH_SOURCE

