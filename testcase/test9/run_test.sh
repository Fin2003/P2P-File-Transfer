#!/bin/bash

# read the arguments from .in
output=$(cat bpkg_test9.in | xargs ../../pkgmain)

# read the expected output
expected_output=$(cat bpkg_test9.out)

# compare the output with the expected output
if [ "$output" == "$expected_output" ]; then
    echo "P1 Test9: Check the leaf node inside last non-leaf node passed."
else
    echo "P1 Test9: Check the leaf node inside last non-leaf node failed."
fi