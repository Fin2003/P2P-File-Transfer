#!/bin/bash

# clecn the output file
> btide_test2.out


{
  echo "CONNECT 127.0.0.1:8800"
  sleep 0.5
  echo "QUIT"  
} | ../../btide config2.cfg >> btide_test2.out

# compare the output with the expected output
if cmp -s btide_test2.out btide_test2.exc; then
  echo "P2 Test12: switch listen port passed."
else
  echo "P2 Test12: switch listen port failed."
fi

rm -rf "tests"