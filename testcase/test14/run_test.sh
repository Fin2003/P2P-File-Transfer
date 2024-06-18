#!/bin/bash

# clecn the output file
> btide_test4.out


{
  echo "CONNECT 127.0.0.1:8080"
  sleep 0.2
  echo "ADDPACKAGE file1.bpkg"
  echo "PACKAGES"
  sleep 0.2
  echo "QUIT"  
} | ../../btide config4.cfg >> btide_test4.out

# compare the output with the expected output
if cmp -s btide_test4.out btide_test4.exc; then
  echo "P2 Test14: data file broken passed."
else
  echo "P2 Test14: data file broken failed."
fi