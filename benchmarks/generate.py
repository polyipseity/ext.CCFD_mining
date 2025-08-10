from dataclasses import dataclass
from pathlib import Path
from random import Random
from types import MappingProxyType
from typing import Iterator


@dataclass(frozen=True, kw_only=True, slots=True)
class GeneratedDatum:
    arity: int
    size: int
    cf: float


GENERATED_DATA = MappingProxyType(
    {
        f"{arity}×{size}; cf={cf}": GeneratedDatum(
            arity=arity,
            size=size,
            cf=cf,
        )
        for arity in (10, 20, 50)
        # for arity in (1, 2, 5, 10, 20, 50)
        for size in (1000, 10000)
        # for size in (100, 1000, 10000)
        for cf in (0.5, 0.7)
        # for cf in (0.1, 0.3, 0.5, 0.7, 0.9)
    }
)


def main() -> None:
    cwd_path = Path("./")
    for folder_name, args in GENERATED_DATA.items():
        folder_path = cwd_path / folder_name
        folder_path.mkdir(exist_ok=True)

        data_file_path = folder_path / "data.csv"

        with data_file_path.open("wt", encoding="UTF-8") as data_file:
            for line in generate_data_lines(
                args.arity, args.size, correlation_factor=args.cf
            ):
                data_file.write(line)
                data_file.write("\n")
            data_file.truncate()


def generate_data_lines(
    arity: int,
    size: int,
    *,
    header: bool = True,
    correlation_factor: float = 0.3,
    seed: int = 42,
) -> Iterator[str]:
    random = Random(seed)
    possible_values_count = max(2, int(size ** (1 / arity)))
    possible_tuples_count = int(size * (1 - correlation_factor))
    possible_tuples = tuple(
        ",".join(str(random.randrange(possible_values_count)) for _ in range(arity))
        for _ in range(possible_tuples_count)
    )

    if header:
        yield ",".join(map(str, range(arity)))
    for _ in range(size):
        yield possible_tuples[random.randrange(possible_tuples_count)]


if __name__ == "__main__":
    main()
