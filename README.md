# Large-Data-Sorting-with-Memory-Constraints
Large Data Sorting with Memory Constraints

Overview

This program sorts a large binary file containing 64-bit integers in ascending order while using a limited amount of virtual memory. The implementation utilizes memory-mapped files (mmap on Linux/macOS, MapViewOfFile on Windows) for efficiency instead of direct file reads and writes. The total memory usage for mapping is constrained to 1/10 of the file size, ensuring that even large files can be sorted without exceeding memory limits.

Features

Memory-efficient sorting: Uses external multi-way merge sort.

Configurable memory limit: Defaults to 1/10 of file size but can be overridden.

File generation: Generates random or pre-sorted binary files for testing.

File validation: Checks if a file is already sorted.

Error handling: Detects invalid file formats, incorrect memory limits, and other common issues.

Program Flow

File Generation (Optional):

Generates a binary file with random 64-bit integers.

Supports an option to generate already sorted data.

Sorting Process:

Splitting into chunks: The file is divided into memory-constrained chunks, each of which is sorted and stored as a run.

Multi-way merge: K-way merging is performed iteratively to combine sorted runs into fewer, larger runs until the entire file is sorted.

Memory constraints are enforced throughout the sorting process.

Validation:

Checks if a file is sorted using a sequential scan.

Command-Line Usage

$ sort_data --gen <filename> <count> [sorted]
$ sort_data --check <filename>
$ sort_data <filename> [memory_limit_MB]

Examples

Generate a file with 1 million random 64-bit integers:

$ sort_data --gen data.bin 1000000

Generate a file with 1 million pre-sorted 64-bit integers:

$ sort_data --gen data.bin 1000000 sorted

Check if a file is sorted:

$ sort_data --check data.bin

Sort a file with the default memory limit (1/10th of file size):

$ sort_data data.bin

Sort a file with a 100MB memory limit:

$ sort_data data.bin 100

Compilation

$ g++ -std=c++17 -o sort_data sort_data.cpp

Test Cases

A script (run_tests.sh) automates testing:

#!/bin/bash
set -e

echo "================= Compilation ================="
g++ -std=c++17 -o sort_data sort_data.cpp
echo "✅ Compilation successful!"

echo "================= Test 1: Generate Random Data ================="
./sort_data --gen random_data.bin 1000000
./sort_data --check random_data.bin
./sort_data random_data.bin 100
./sort_data --check random_data.bin

echo "================= Test 2: Already Sorted Data ================="
./sort_data --gen sorted_data.bin 1000000 sorted
./sort_data --check sorted_data.bin
./sort_data sorted_data.bin

echo "================= Test 3: Empty File ================="
touch empty.bin
./sort_data --check empty.bin
./sort_data empty.bin

echo "================= Test 4: Single-Element File ================="
echo -n -e "\x01\x00\x00\x00\x00\x00\x00\x00" > single.bin
./sort_data --check single.bin
./sort_data single.bin

echo "================= Test 5: Two-Element Unsorted File ================="
echo -n -e "\x02\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00" > small.bin
./sort_data --check small.bin
./sort_data small.bin
./sort_data --check small.bin

echo "================= Test 6: Large Dataset (10 million numbers) ================="
./sort_data --gen large_data.bin 10000000
./sort_data --check large_data.bin
./sort_data large_data.bin 100
./sort_data --check large_data.bin

echo "================= Test 7: Invalid Inputs ================="
./sort_data sorted_data.bin abc
./sort_data

echo "✅ All tests completed successfully!"

Implementation Details

Sorting Algorithm

This program uses an external multi-way merge sort:

Initial Run Generation: The file is divided into chunks small enough to fit within the memory limit. Each chunk is sorted and written as a run.

Multi-way Merging:

Uses a min-heap (priority queue) to merge K sorted runs at a time.

Each pass reduces the number of runs until only one remains.

Memory Optimization:

Uses mmap to map parts of the file into memory instead of loading it all at once.

Ensures that memory usage does not exceed the specified limit.

File Handling

Files are processed in binary mode to handle raw 64-bit integers efficiently.

The implementation does not assume text-based formats, ensuring optimal performance.

Error Handling

Invalid file format detection: Ensures the file contains only 64-bit integers.

Memory limit validation: Rejects out-of-range or non-numeric limits.

Graceful error messages: Provides user-friendly diagnostics when failures occur.


This program efficiently sorts large datasets while maintaining strict memory constraints. It supports configurable memory limits, integrates file validation, and provides automated testing. The use of mmap ensures high performance by avoiding excessive file I/O operations.

