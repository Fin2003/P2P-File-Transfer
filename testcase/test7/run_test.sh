#!/bin/bash

# read the arguments from .in
output=$(cat bpkg_test7.in | xargs ../../pkgmain)

# read the expected output
expected_output=$(cat bpkg_test7.out)

# compare the output with the expected output
if [ "$output" == "$expected_output" ]; then
    echo "P1 Test7: ident not found passed."
else
    echo "P1 Test7: ident not found failed."
fi