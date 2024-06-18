#!/bin/bash

# clecn the output file
> btide_test5.out


{
  echo "ADDPACKAGE file1.bpkg"
  sleep 0.1
  echo "ADDPACKAGE complete_3.bpkg"
  sleep 0.1
  echo "ADDPACKAGE incomplete_1.bpkg"
  sleep 0.1
  echo "ADDPACKAGE file1.bpkg"
  sleep 0.1
  echo "ADDPACKAGE file1.bpkg"
  sleep 0.1
  echo "PACKAGES"
  sleep 0.1
  echo "REMPACKAGE e370a823bf279694ddb22af800dcaad9e498ccb8bdf538905537f2"
  sleep 0.1
  echo "REMPACKAGE e370a823bf279"
  sleep 0.1
  echo "REMPACKAGE"
  sleep 0.1
  echo "PACKAGES"
  sleep 0.1
  echo "QUIT"  
} | ../../btide config5.cfg >> btide_test5.out

# compare the output with the expected output
if cmp -s btide_test5.out btide_test5.exc; then
  echo "P2 Test15: Add same package and Remove with 2 wrong command passed."
else
  echo "P2 Test15: Add same package and Remove with 2 wrong command failed."
fi

echo "...testcase 16 may need some second...."