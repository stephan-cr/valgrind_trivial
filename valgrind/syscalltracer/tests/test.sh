#!/bin/sh

option=""
program=./sample

../../usr/bin/valgrind --tool=syscalltracer $* $program 2>&1

