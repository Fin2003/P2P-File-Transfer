#!/bin/bash

# clecn the output file
> btide_test6.out

{
  echo "CONNECT 127.0.0.1:23456"
  sleep 0.2
  echo "CONNECT 127.0.0.1:9000"
  sleep 0.2
  echo "DISCONNECT 127.0.0.1:9000"
  sleep 0.2
  echo "CONNECT 127.0.0.1:23456"
  sleep 0.2
  echo "PEERS"
  sleep 0.2
  echo "DISCONNECT 127.0.0.1:23456"
  sleep 0.2
  echo "CONNECT 127.0.0.1:5555"
  sleep 0.2
  echo "PEERS"
  sleep 0.2
  echo "FETCH"
  sleep 0.2
  echo "DISCONNECT 127.0.0.1:23456"
  sleep 0.2
  echo "PEERS"
  sleep 0.2
  echo "QUIT"  
} | ../../btide config6.cfg >> btide_test6.out

# compare the output with the expected output
if cmp -s btide_test6.out btide_test6.exc; then
  echo "P2 Test16: loop of connect and disconnect and PEERS of multi address passed."
else
  echo "P2 Test16: loop of connect and disconnect and PEERS of multi address failed."
fi



  