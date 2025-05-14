# CCFD Mining

## Compile
- Step 1: `cmake .`  
- Step 2: `make`
- The executable file is `CFDMiner` and `FGC_Stream_CFDMiner`

## GC-growth + CFDMiner
A classic CCFD mining method based on the GC-growth algorithm and the CFDMiner algorithm.

The executable file is `CFDMiner`, corresponding to `main_CFDMiner.cpp`.  
Usage: `./CFDMiner [csv_file_path] [minsupp] [maxsize]`  
where:
- `csv_file_path` is the path to the input comma-separated CSV data file.
- `minsupp` is the minimum support threshold, which is a float number in the range [0, 1].
- `maxsize` is the maximum size of the itemset, which is an integer number.  

The output CCFDs are stored in the file `CFDMiner_ccfd.txt` in the same directory as the executable file.

## FGC-Stream + CFDMiner
A CCFD mining method based on the FGC-Stream algorithm and the CFDMiner algorithm, where the FGC-Stream algorithm is a stream-based algorithm for mining frequent closed and free itemsets.

The executable file is `FGC_Stream_CFDMiner`, corresponding to `FGC-Stream/main.cpp`.  
Usage: `./FGC_Stream_CFDMiner [csv_file_path] [minsupp] [window_size] [exit_at]`  
where:
- `csv_file_path` is the path to the input comma-separated CSV data file.
- `minsupp` is the minimum support threshold, which is a float number in the range [0, 1].
- `window_size` is the size of the sliding window, which is an integer number.
- `exit_at` is the number of transactions to process before exiting, which is an integer number.

The output CCFDs are stored in the file `FGC_Stream_CFDMiner_ccfd.txt` in the same directory as the executable file.

## CFDMiner_Graph
A CCFD mining method based on the CFDMiner algorithm and the graph-based algorithm, where the graph-based algorithm is a method for collecting the CCFDs from several the datasets.
This method is suitable for the case where the dataset is stored in multiple files.

The executable file is `CFDMiner_Graph`, corresponding to `main_graph.cpp`.  
Usage: `./CFDMiner_Graph [csv_files_folder] [minsupp] [maxsize]`  
where:
- `csv_files_folder` is the path to the folder containing the input comma-separated CSV data files.
- `minsupp` is the minimum support threshold, which is a float number in the range [0, 1].
- `maxsize` is the maximum size of the itemset, which is an integer number.

The output CCFDs are stored in the file `CFDMiner_Graph_ccfd.txt` in the same directory as the executable file.