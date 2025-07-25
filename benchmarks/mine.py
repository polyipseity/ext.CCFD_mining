from glob import iglob
from itertools import product
from os import name
from pathlib import Path
from re import A
from shutil import move
from subprocess import check_call
from sys import stderr, stdout
from time import monotonic_ns
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
CFD_MINER = (
    EXECUTABLE_FOLDER_PATH / f"CFDMiner{'.exe' if name == 'nt' else ''}"
)  # CFDMiner [csv_file_path] [minsupp] [maxsize]
STREAM_MINER = (
    EXECUTABLE_FOLDER_PATH / f"FGC_Stream_CFDMiner{'.exe' if name == 'nt' else ''}"
)  # FGC_Stream_CFDMiner [csv_file_path] [minsupp] [window_size] [exit_at]
CFD_MINER_GRAPH = (
    EXECUTABLE_FOLDER_PATH / f"CFDMiner_Graph{'.exe' if name == 'nt' else ''}"
)  # CFDMiner_Graph [csv_files_folder] [minsupp] [maxsize]

INPUT_PATH_PLACEHOLDER = object()
BENCHMARKS = MappingProxyType(
    {
        **{
            f"support={sup}": (
                CFD_MINER,
                INPUT_PATH_PLACEHOLDER,
                str(sup),
                str(255),
            )
            for sup in (0.1, 0.05, 0.01, 0.005)
        },
        **{
            f"stream, support={sup}, window={win}": (
                STREAM_MINER,
                INPUT_PATH_PLACEHOLDER,
                str(sup),
                str(win),
            )
            for sup in (0.1, 0.05, 0.01, 0.005)
            for win in (1000, 2000, 5000, 10000)
        },
        **{
            f"graph, support={sup}, window={win}": (
                STREAM_MINER,
                INPUT_PATH_PLACEHOLDER,
                str(sup),
                str(win),
            )
            for sup in (0.1, 0.05, 0.01, 0.005)
            for win in (1000, 2000, 5000, 10000)
        },
        **{
            f"stream, support={sup}": (
                CFD_MINER_GRAPH,
                INPUT_PATH_PLACEHOLDER,
                str(sup),
            )
            for sup in (0.1, 0.05, 0.01, 0.005)
        },
    }
)


def main() -> None:
    for data_csv_filepath in iglob("*/*.csv"):
        data_csv_filepath = Path(data_csv_filepath).resolve(strict=True)
        cwd = data_csv_filepath.parent

        for name, benchmark in BENCHMARKS.items():
            result_folder_path = cwd / f"ret; {name}"
            if (result_folder_path / ".ignore").exists():
                continue
            result_folder_path.mkdir(exist_ok=True)

            def args0(args=benchmark):
                for arg in args:
                    if arg is INPUT_PATH_PLACEHOLDER:
                        yield data_csv_filepath
                        continue
                    if isinstance(arg, (Path, str)):
                        yield arg
                        continue
                    raise ValueError(arg)

            args = tuple(args0())
            start_time_ns = monotonic_ns()
            print(f"start: {args}")
            check_call(args, cwd=cwd, stdin=None, stdout=stdout, stderr=stderr)
            end_time_ns = monotonic_ns()
            elapsed_ns = end_time_ns - start_time_ns
            print(f"end: used {elapsed_ns} ns")

            (result_folder_path / "performance.txt").write_text(
                f"elapsed: {elapsed_ns} ns\n"
            )
            for result_filename in iglob("*.txt", root_dir=cwd):
                move(cwd / result_filename, result_folder_path / result_filename)


if __name__ == "__main__":
    main()
