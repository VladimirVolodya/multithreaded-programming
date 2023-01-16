#!/bin/bash

echo "With mutex:"
g++ -pthread bench.cpp
for i in 1 2 4 8 16 32 64 128 256 512 1024
do
    ./a.out $i 1000
done

echo "Lock-free"
g++ -pthread bench.cpp -DLOCK_FREE
for i in 1 2 4 8 16 32 64 128 256 512 1024
do
    ./a.out $i 1000
done
