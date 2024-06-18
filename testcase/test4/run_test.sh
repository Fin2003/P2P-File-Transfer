#!/bin/bash

# read the arguments from .in
output=$(cat bpkg_test4.in | xargs ../../pkgmain)

# read the expected output
expected_output=$(cat bpkg_test4.out)

# compare the output with the expected output
if [ "$output" == "$expected_output" ]; then
    echo "P1 Test4: file name missing passed."
else
    echo "P1 Test4: file name missing failed."
fi