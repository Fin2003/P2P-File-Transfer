#!/bin/bash

# read the arguments from .in
output=$(cat bpkg_test3.in | xargs ../../pkgmain)

# read the expected output
expected_output=$(cat bpkg_test3.out)

# compare the output with the expected output
if [ "$output" == "$expected_output" ]; then
    echo "P1 Test3: chunks in bpkg file be change passed."
else
    echo "P1 Test3: chunks in bpkg file be change failed."
fi