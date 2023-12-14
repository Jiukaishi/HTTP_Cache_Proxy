#!/bin/bash
make clean
make 
echo 'running proxy as daemon...'
./main &
while true ; do continue ; done
