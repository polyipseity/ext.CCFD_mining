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
        for arity in (1, 2, 5, 10, 20, 50)
        for size in (100, 1000, 10000)
        # for cf in (0.1, 0.3, 0.5)
        for cf in (0.5,)
    }
)


def main() -> None:
    cwd_path = Path("./")
    for folder_name, args in GENERATED_DATA.items():
        folder_path = cwd_path / folder_name
        folder_path.mkdir()

        data_file_path = folder_path / "data.csv"

        with data_file_path.open("wt", encoding="UTF-8") as data_file:
            for line in generate_data_lines(
                args.arity, args.size, correlation_factor=args.cf
            ):
                data_file.write(line)
                data_file.write("\n")


def generate_data_lines(
    arity: int,
    size: int,
    *,
    header: bool = True,
    correlation_factor: float = 0.3,
    seed: int = 42,
) -> Iterator[str]:
    random = Random(seed)
    possible_values_num = int(size * (1 - correlation_factor))

    if header:
        yield ",".join(map(str, range(arity)))
    for _ in range(size):
        yield ",".join(str(random.randrange(possible_values_num)) for _ in range(arity))


if __name__ == "__main__":
    main()
