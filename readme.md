# CCFD Mining

## Compile
- Step 1: `cmake .`  
- Step 2: `make`
- The executable file is `CFDMiner` and `FGC_Stream_CFDMiner`

## GC-growth + CFDMiner
The executable file is `CFDMiner`, corresponding to `main_CFDMiner.cpp`.  
Usage: `./CFDMiner [csv_file_path] [minsupp] [maxsize]`  
where:
- `csv_file_path` is the path to the input comma-separated CSV data file.
- `minsupp` is the minimum support threshold, which is a float number in the range [0, 1].
- `maxsize` is the maximum size of the itemset, which is an integer number.

## FGC-Stream + CFDMiner
The executable file is `FGC_Stream_CFDMiner`, corresponding to `FGC-Stream/main.cpp`.  
Usage: `./FGC_Stream_CFDMiner [csv_file_path] [minsupp] [window_size] [exit_at]`  
where:
- `csv_file_path` is the path to the input comma-separated CSV data file.
- `minsupp` is the minimum support threshold, which is a float number in the range [0, 1].
- `window_size` is the size of the sliding window, which is an integer number.
- `exit_at` is the number of transactions to process before exiting, which is an integer number.