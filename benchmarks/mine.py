from os import name
from pathlib import Path
from types import MappingProxyType

"""
where
- `csv_file_path` is the path to the input comma-separated CSV data file.
- `csv_files_folder` is the path to the folder containing the input comma-separated CSV data files.
- `minsupp` is the minimum support threshold, which is a float number in the range [0, 1].
- `maxsize` is the maximum size of the itemset, which is an integer number.
- `window_size` is the size of the sliding window, which is an integer number.
- `exit_at` is the number of transactions to process before exiting, which is an integer number.
"""

EXECUTABLE_FOLDER_PATH = Path("../build/Debug/")
CFD_MINER = f"CFDMiner{'.exe' if name == 'nt' else ''}"  # CFDMiner [csv_file_path] [minsupp] [maxsize]
STREAM_MINER = f"FGC_Stream_CFDMiner{'.exe' if name == 'nt' else ''}"  # FGC_Stream_CFDMiner [csv_file_path] [minsupp] [window_size] [exit_at]
STREAM_MINER_GRAPH = f"CFDMiner_Graph{'.exe' if name == 'nt' else ''}"  # CFDMiner_Graph [csv_files_folder] [minsupp] [maxsize]

INPUT_PATH_PLACEHOLDER = object()
BENCHMARKS = MappingProxyType(
    {
        "support=0.005": (CFD_MINER, INPUT_PATH_PLACEHOLDER, str(0.005)),
        "support=0.01": (CFD_MINER, INPUT_PATH_PLACEHOLDER, str(0.01)),
        "support=0.05": (CFD_MINER, INPUT_PATH_PLACEHOLDER, str(0.05)),
        "support=0.1": (CFD_MINER, INPUT_PATH_PLACEHOLDER, str(0.1)),
    }
)


def main() -> None: ...


if __name__ == "__main__":
    main()
