#!/bin/bash

# clecn the output file
> btide_test3.out


{
  echo "CONNECT 127.0.0.1:8800"
  sleep 0.2
  echo "CONNECT 127.0.0.1:8800"
  sleep 0.2
  echo "QUIT"  
} | ../../btide config3.cfg >> btide_test3.out

# compare the output with the expected output
if cmp -s btide_test3.out btide_test3.exc; then
  echo "P2 Test13: repeat connect to the same peer passed."
else
  echo "P2 Test13: repeat connect to the same peer failed."
fi

rm -rf "tests"