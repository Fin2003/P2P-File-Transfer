#!/bin/bash

# read the arguments from .in
output=$(cat bpkg_test10.in | xargs ../../pkgmain)

# read the expected output
expected_output=$(cat bpkg_test10.out)

# compare the output with the expected output
if [ "$output" == "$expected_output" ]; then
    echo "P1 Test10: two chunks create tree passed."
else
    echo "P1 Test10: two chunks create tree failed."
fi