#!/bin/sh

program=./remap
testfile=8k

if [ "$1" ] ; then
  program=$1
fi

dd if=/dev/zero of=$testfile bs=1024 count=8 2> /dev/null
../../usr/bin/valgrind --tool=ml $program $testfile 2>&1
rm -f $testfile
