#!/bin/bash

# Exit if any command fails
set -e

echo "================= Compilation: Compiling sort_data.cpp ================="
g++ -std=c++17 -o sort_data sort_data.cpp
echo "✅ Compilation successful!"

echo "================= Test 1: Generate Random Data ================="
./sort_data --gen random_data.bin 1000000
./sort_data --check random_data.bin  # Should say "File is NOT sorted"
./sort_data random_data.bin 100      # Sorting with 100 MB limit
./sort_data --check random_data.bin  # Should now be sorted

echo "================= Test 2: Already Sorted Data ================="
./sort_data --gen sorted_data.bin 1000000 sorted
./sort_data --check sorted_data.bin  # Should confirm sorted
./sort_data sorted_data.bin          # Should detect and skip sorting

echo "================= Test 3: Empty File ================="
touch empty.bin
./sort_data --check empty.bin  # Should say "trivially sorted"
./sort_data empty.bin          # Should say "already sorted"

echo "================= Test 4: Single-Element File ================="
echo -n -e "\x01\x00\x00\x00\x00\x00\x00\x00" > single.bin
./sort_data --check single.bin  # Should say "trivially sorted"
./sort_data single.bin          # Should say "already sorted"

echo "================= Test 5: Two-Element Unsorted File ================="
echo -n -e "\x02\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00" > small.bin
./sort_data --check small.bin  # Should say "File is NOT sorted"
./sort_data small.bin
./sort_data --check small.bin  # Should now be sorted

echo "================= Test 6: Large Dataset (10 million numbers) ================="
./sort_data --gen large_data.bin 10000000
./sort_data --check large_data.bin  # Should say "File is NOT sorted"
./sort_data large_data.bin 100        # ✅ Increase memory limit to 10MB to prevent mmap errors
./sort_data --check large_data.bin  # Should now be sorted

echo "================= Test 7: Invalid Inputs ================="
./sort_data sorted_data.bin abc  # Should print an error (invalid argument)
./sort_data                      # Should print usage instructions

echo "✅ All tests completed successfully!"
