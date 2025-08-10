from pathlib import Path
from ucimlrepo import fetch_ucirepo


def main() -> None:
    for id, name in {
        2: "Adult",  # <https://archive.ics.uci.edu/dataset/2/adult>
        19: "Car Evaluation",  # <https://archive.ics.uci.edu/dataset/19/car+evaluation>
        23: "Chess (King-Rook vs. King)",  # <https://archive.ics.uci.edu/dataset/23/chess+king+rook+vs+king>
    }.items():
        fetch(id, name=name)


def fetch(id: int, *, name: str) -> None:
    folder_path = Path(name)
    data_path = folder_path / "data.csv"
    if data_path.exists():
        return

    dataset = fetch_ucirepo(id=id)
    folder_path.mkdir(exist_ok=True)
    dataset.data.original.to_csv(data_path, index=False)


if __name__ == "__main__":
    main()
