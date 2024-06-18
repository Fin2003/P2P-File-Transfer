#!/bin/bash

# read the arguments from .in
output=$(cat bpkg_test5.in | xargs ../../pkgmain)

# read the expected output
expected_output=$(cat bpkg_test5.out)

# compare the output with the expected output
if [ "$output" == "$expected_output" ]; then
    echo "P1 Test5: file name over size passed."
else
    echo "P1 Test5: file name over size failed."
fi