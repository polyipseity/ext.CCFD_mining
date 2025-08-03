from dataclasses import dataclass
from glob import iglob
from itertools import zip_longest
from json import dump, load
from os import name
from pathlib import Path, PurePath
from shutil import move
from subprocess import check_call
from sys import stderr, stdout
from tempfile import TemporaryDirectory
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
)  # CFDMiner_Graph [minsupp] [maxsize] [csv_files_folder...]
MAX_ITEM_SET_SIZE = 255

INPUT_PATH_PLACEHOLDER = object()


@dataclass(frozen=True, kw_only=True, slots=True)
class SameAbsoluteSupportPlaceholder:
    support: float
    window: int


@dataclass(frozen=True, kw_only=True, slots=True)
class PartitionedInputPathsPlaceholder:
    window: int


BENCHMARKS = MappingProxyType(
    {
        **{
            f"support={sup}": (
                CFD_MINER,
                INPUT_PATH_PLACEHOLDER,
                str(sup),
                str(MAX_ITEM_SET_SIZE),
            )
            for sup in (0.1, 0.05, 0.01, 0.005)
        },
        **{
            f"stream, support={sup}, window={win}": (
                STREAM_MINER,
                INPUT_PATH_PLACEHOLDER,
                SameAbsoluteSupportPlaceholder(support=sup, window=win),
                str(win),
            )
            for sup in (0.1, 0.05, 0.01, 0.005)
            for win in (1000, 2000, 5000, 10000)
        },
        **{
            f"graph, support={sup}, window={win}": (
                CFD_MINER_GRAPH,
                SameAbsoluteSupportPlaceholder(support=sup, window=win),
                str(MAX_ITEM_SET_SIZE),
                PartitionedInputPathsPlaceholder(window=win),
            )
            for sup in (0.1, 0.05, 0.01, 0.005)
            for win in (1000, 2000, 5000, 10000)
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

            with TemporaryDirectory() as temp_dir_path:
                temp_dir_path = Path(temp_dir_path)

                def args0(
                    args=benchmark,
                    data_csv_filepath=data_csv_filepath,
                    temp_dir_path=temp_dir_path,
                ):
                    temp_file_idx = 0
                    for arg in args:
                        if arg is INPUT_PATH_PLACEHOLDER:
                            yield data_csv_filepath
                            continue
                        if isinstance(arg, SameAbsoluteSupportPlaceholder):
                            with data_csv_filepath.open("rb") as data_csv_file:
                                data_size = max(0, sum(1 for _ in data_csv_file) - 1)
                            absolute_support = data_size * arg.support
                            yield str(absolute_support / arg.window)
                            continue
                        if isinstance(arg, PartitionedInputPathsPlaceholder):
                            with data_csv_filepath.open("rb") as data_csv_file:
                                try:
                                    header = next(data_csv_file)
                                except StopIteration:
                                    continue
                                for lines in zip_longest(
                                    *((data_csv_file,) * arg.window)
                                ):
                                    content = b"".join(
                                        line for line in lines if line is not None
                                    )
                                    if content:
                                        temp_filepath = temp_dir_path / str(
                                            temp_file_idx
                                        )
                                        temp_filepath.write_bytes(header + content)
                                        temp_file_idx += 1
                                        yield temp_filepath
                            continue
                        if isinstance(arg, (PurePath, str)):
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

            for result_filename in iglob("*.txt", root_dir=cwd):
                if result_filename == "converted_dataset.txt":
                    continue
                move(cwd / result_filename, result_folder_path / result_filename)

            performance_filepath = result_folder_path / "performance.json"
            if not performance_filepath.exists():
                performance_filepath.write_bytes(b"{}")
            with performance_filepath.open("r+t", encoding="UTF-8") as performance_file:
                performance = load(performance_file)

                try:
                    performance["elapsed"]
                except KeyError:
                    performance["elapsed"] = []
                performance["elapsed"].append(elapsed_ns)
                performance_file.seek(0)

                dump(
                    performance,
                    performance_file,
                    ensure_ascii=False,
                    check_circular=False,
                    indent=4,
                    sort_keys=True,
                )
                performance_file.truncate()

            (result_folder_path / ".ignore").write_bytes(b"")


if __name__ == "__main__":
    main()
