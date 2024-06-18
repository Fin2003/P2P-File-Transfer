#!/bin/bash

# read the arguments from .in
output=$(cat bpkg_test1.in | xargs ../../pkgmain)

# read the expected output
expected_output=$(cat bpkg_test1.out)

# compare the output with the expected output
if [ "$output" == "$expected_output" ]; then
    echo "P1 Test1: Over hahes/chunks passed."
else
    echo "P1 Test1: Over hahes/chunks failed."
fi