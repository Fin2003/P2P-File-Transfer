#!/bin/bash

# read the arguments from .in
output=$(cat bpkg_test8.in | xargs ../../pkgmain)

# read the expected output
expected_output=$(cat bpkg_test8.out)

# compare the output with the expected output
if [ "$output" == "$expected_output" ]; then
    echo "p1 Test8: Merkle_nchunks and nhashes difference more than 1 passed."
else
    echo "p1 Test8: Merkle_nchunks and nhashes difference more than 1 failed."
fi