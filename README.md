
# **Large Data Sorting with Memory Limit (External Merge Sort using mmap)**

This project implements an **efficient external sorting algorithm** for a large binary file containing **64-bit integers**. The sorting is performed **within a specified memory limit**, using **memory-mapped files (mmap on Linux/macOS, MapViewOfFile on Windows)** for optimized file access.

## **Features**
- Sorts large binary files containing **64-bit integers**.
- Uses **multi-way merge sorting** with **memory mapping (mmap)**.
- Limits virtual memory usage to **1/10 of the file size** (by default).
- Supports **custom memory limit** (user-defined in MB).
- Includes utilities to **generate test files** and **verify sorting results**.
- Detects **already sorted files** and skips sorting.

---

## **Usage**
### **Compiling the Code**
Ensure you have **g++ (C++17)** installed, then compile:

```sh
g++ -std=c++17 -o sort_data sort_data.cpp
```

---

### **Running the Program**
#### **1. Generate a test file**
Generate a file with **1,000,000 random 64-bit integers**:
```sh
./sort_data --gen datafile.bin 1000000
```
Generate a **pre-sorted** file:
```sh
./sort_data --gen datafile.bin 1000000 sorted
```

#### **2. Check if a file is sorted**
```sh
./sort_data --check datafile.bin
```

#### **3. Sort a file**
Sort a file with the **default memory limit** (1/10th of file size):
```sh
./sort_data datafile.bin
```
Sort a file with a **custom memory limit** (e.g., **100MB**):
```sh
./sort_data datafile.bin 100
```

---

## **Algorithm**
The sorting is performed using a **multi-phase external merge sort**:
1. **Chunk Sorting**: The file is split into chunks based on available memory, each chunk is **sorted individually** and written back as a "run".
2. **Multi-Way Merge**: Several sorted chunks (runs) are merged iteratively until **only one fully sorted file** remains.
3. **Memory Management**: Uses **mmap** to read and write chunks efficiently while respecting the memory limit.

---

## **Test Cases**
Run the provided test script:
```sh
./run_tests.sh
```
Expected test results:
1. ✅ Sorting works correctly for **random and pre-sorted data**.
2. ✅ **Already sorted files** are detected and skipped.
3. ✅ **Empty and single-element files** are handled correctly.
4. ✅ **Two-element files (unsorted)** are sorted properly.
5. ✅ **Large files (10M integers)** sort successfully within memory limits.
6. ✅ **Invalid input handling** (wrong memory limit, missing arguments).

---

## **Example Output**
```
✅ Compilation successful!
================= Test 1: Generate Random Data =================
Generated 1000000 64-bit integers into random_data.bin.
File is NOT sorted (found -5804827286701746495 after 4790948568885837551).
Checking if the file is already sorted...
File has 1000000 elements (8000000 bytes). Using up to 100 MB of mmap.
🔹 Starting external multi-way mergesort...
Sorting complete.
File is sorted ascending.
...
✅ All tests completed successfully!
```

---

## **System Requirements**
- **Linux/macOS** (Uses `mmap`)
- **Windows** (Uses `MapViewOfFile` but requires adjustments)
- **C++17** compiler (**g++**, **Clang**)
- **At least 2GB RAM recommended** for large files
