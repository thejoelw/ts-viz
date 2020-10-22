#!/bin/sh

rm -f progs.jsons && mkfifo progs.jsons && nodemon --quiet program/main.js > progs.jsons
