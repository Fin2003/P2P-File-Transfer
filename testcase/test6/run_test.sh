#!/bin/bash

# read the arguments from .in
output=$(cat bpkg_test6.in | xargs ../../pkgmain)

# read the expected output
expected_output=$(cat bpkg_test6.out)

# compare the output with the expected output
if [ "$output" == "$expected_output" ]; then
    echo "P1 Test6: nhashes is zero passed."
else
    echo "P1 Test6: nhashes is zero failed."
fi