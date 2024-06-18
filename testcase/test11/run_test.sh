#!/bin/bash

# Get the first line as command line argument
cmd_arg=$(head -n 1 btide_test1.in)

# Loop through each line of the input file starting from the second line
tail -n +2 btide_test1.in | while IFS= read -r line; do
    printf "%s\n" "$line" | ../../btide "$cmd_arg" >> btide_test1.txt
done

if [ -d "tesdfhjsdfkjashdflhsdalfhewlkjhfdlsajhfldshfljhlasjdhfljsdhflhawlfeuiahldjfhlsdflhsdflds" ]; then
  echo "P2 Test11: create directory passed."
  rm -rf "tesdfhjsdfkjashdflhsdalfhewlkjhfdlsajhfldshfljhlasjdhfljsdhflhawlfeuiahldjfhlsdflhsdflds"
else
  echo "P2 Test11: create directory failed."
fi