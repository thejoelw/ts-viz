#!/bin/sh

sudo apt-get install tup
sudo apt-get install libfftw3-dev

tup init
tup variant configs/*.config
tup -j8