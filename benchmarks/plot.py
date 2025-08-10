from collections import defaultdict
from dataclasses import dataclass
from glob import iglob
from json import dump, load
from os import scandir
from pathlib import Path
from types import MappingProxyType
from typing import Any, Mapping, Self

from matplotlib.pyplot import close, figure
from matplotlib.ticker import FuncFormatter, MaxNLocator
import numpy as np


@dataclass(frozen=True, kw_only=True, slots=True)
class Rule:
    inputs: frozenset[tuple[str, str]]
    output: tuple[str, str]
    support: int

    @classmethod
    def parse(cls, line: str) -> Self:
        rule, vals = line.split(", (", 1)
        vals, stats = vals.split("), ", 1)

        inputs = {
            col: val
            for col, val in zip(
                rule[len("[") : rule.index("] => ")].split(", "),
                vals[: vals.index(" || ")].split(", "),
                strict=True,
            )
        }
        output = (
            rule[rule.index("] => ") + len("] => ") :],
            vals[vals.index(" || ") + len(" || ") :],
        )
        support = int(stats[len("support :") :])
        support = 0  # ignore support for equality checking

        return cls(inputs=frozenset(inputs.items()), output=output, support=support)


def parse_folder_name(name: str) -> tuple[str, dict[str, float | int]]:
    type, parts = name.split("; ", 1)
    parameters = dict[str, float | int]()
    for part in parts.split(", "):
        if "=" not in part:
            type = part
            continue
        key, val = part.split("=", 1)
        try:
            parameters[key] = int(val)
        except ValueError:
            parameters[key] = float(val)
    return type, parameters


def calculate_precision_and_recall() -> None:
    for ccfd_pathname in iglob("**/*ccfd.txt", recursive=True):
        ccfd_path = Path(ccfd_pathname)
        if ccfd_path.name == "CFDMiner_ccfd.txt":
            continue

        _, parameters = parse_folder_name(ccfd_path.parts[-2])
        control_ccfd_path = (
            ccfd_path.parent.with_name(
                f"default; support={round(parameters['support'] * parameters['window'])}"
            )
            / "CFDMiner_ccfd.txt"
        )

        control_ccfds = frozenset(
            Rule.parse(line)
            for line in control_ccfd_path.read_text().splitlines()
            if line
        )
        ccfds = frozenset(
            Rule.parse(line) for line in ccfd_path.read_text().splitlines() if line
        )
        matching_ccfds = control_ccfds & ccfds

        performance_filepath = ccfd_path.with_name("performance.json")
        if not performance_filepath.exists():
            performance_filepath.write_bytes(b"{}")
        with ccfd_path.with_name("performance.json").open(
            "r+t", encoding="UTF-8"
        ) as performance_file:
            performance = load(performance_file)

            performance["precision"] = (
                len(matching_ccfds) / len(ccfds) if len(ccfds) > 0 else -1
            )
            performance["recall"] = (
                len(matching_ccfds) / len(control_ccfds)
                if len(control_ccfds) > 0
                else -1
            )

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


@dataclass(frozen=True, kw_only=True, slots=True)
class Result:
    type: str
    parameters: Mapping[str, float | int]
    performance: Any

    def __post_init__(self) -> None:
        object.__setattr__(self, "parameters", MappingProxyType(dict(self.parameters)))


def main() -> None:
    calculate_precision_and_recall()

    for data_folder in scandir("./"):
        if not data_folder.is_dir():
            continue
        results_folder = Path(data_folder) / "results"
        if not results_folder.exists():
            continue

        typed_results = defaultdict[str, list[Result]](list)
        for result_folder_path in iglob("*", root_dir=results_folder):
            result_folder_path = results_folder / result_folder_path
            result_folder_name = result_folder_path.name

            performance_filepath = result_folder_path / "performance.json"
            try:
                with performance_filepath.open("rt") as file:
                    performance = load(file)
            except FileNotFoundError:
                continue

            type, parameters = parse_folder_name(result_folder_name)
            typed_results[type].append(
                Result(type=type, parameters=parameters, performance=performance)
            )

        fig = figure(figsize=(8, 3))
        try:
            for type_idx, (type, results) in enumerate(typed_results.items()):
                data = defaultdict[str, list[float]](list)
                for result in results:
                    for key, val in result.parameters.items():
                        data[key].append(val)
                    data["elapsed"].append(np.mean(result.performance["elapsed"]) / 1e9)

                """
                support_diff, support_diff_indices = np.unique(
                    data["support"], return_inverse=True
                )
                support_diff = np.diff(support_diff)
                support_diff = np.concatenate(
                    (support_diff, np.mean(support_diff, keepdims=True))
                )
                support_diff = support_diff[support_diff_indices]
                if data["window"]:
                    window_diff, window_diff_indices = np.unique(
                        data["window"], return_inverse=True
                    )
                    window_diff = np.diff(window_diff)
                    window_diff = np.concatenate(
                        (window_diff, np.mean(window_diff, keepdims=True))
                    )
                    window_diff = window_diff[window_diff_indices]
                else:
                    window_diff = 1

                ax = fig.add_subplot(1, len(typed_results), type_idx + 1, projection="3d")
                ax.bar3d(data["support"], data["window"] or -1, 0, support_diff, window_diff, data["elapsed"], shade=True)  # type: ignore
                """

                base = 10
                support_ticks, support = np.unique(data["support"], return_inverse=True)
                window_ticks, window = np.unique(
                    data["window"] or -1, return_inverse=True
                )
                zz = np.log(data["elapsed"]) / np.log(base)

                ax = fig.add_subplot(
                    1, len(typed_results), type_idx + 1, projection="3d"
                )
                xx, yy = np.meshgrid(
                    np.arange(support_ticks.size + 1),
                    np.arange(window_ticks.size + 1),
                )
                ax.bar3d(  # type: ignore
                    support,
                    window,
                    np.minimum(zz, 0),
                    1,
                    1,
                    np.abs(zz),
                    color=np.random.rand(len(zz), 3),
                    shade=True,
                )
                ax.plot_surface(xx, yy, np.zeros_like(xx), color="red")  # type: ignore

                ax.set_title(type)
                ax.set_xlabel("support")
                ax.set_xticks(np.arange(support_ticks.size), support_ticks)
                ax.set_ylabel("window")
                ax.set_yticks(np.arange(window_ticks.size), window_ticks)
                ax.set_zlabel("elapsed")  # type: ignore

                def log_tick_formatter(val, pos=None):
                    return f"${base}^{{{round(val, 2)}}}$"

                ax.zaxis.set_major_formatter(FuncFormatter(log_tick_formatter))  # type: ignore
                ax.zaxis.set_major_locator(MaxNLocator(integer=True))  # type: ignore
            fig.savefig(results_folder / "elapsed.svg")
        finally:
            close(fig)

        fig = figure(figsize=(8, 3))
        try:
            for type_idx, (type, results) in enumerate(typed_results.items()):
                data = defaultdict[str, list[float]](list)
                for result in results:
                    if result.performance.get("precision", -1) == -1:
                        continue
                    if result.performance["precision"] == 0:
                        # If data size is less than window size, our current implementation of the stream miner will produce no rules.
                        continue
                    for key, val in result.parameters.items():
                        data[key].append(val)
                    data["precision"].append(result.performance["precision"])

                support_ticks, support = np.unique(data["support"], return_inverse=True)
                window_ticks, window = np.unique(
                    data["window"] or -1, return_inverse=True
                )
                zz = data["precision"]
                if not zz:
                    continue

                ax = fig.add_subplot(
                    1, len(typed_results), type_idx + 1, projection="3d"
                )
                ax.bar3d(  # type: ignore
                    support,
                    window,
                    0,
                    1,
                    1,
                    zz,
                    color=np.random.rand(len(zz), 3),
                    shade=True,
                )

                ax.set_title(type)
                ax.set_xlabel("support")
                ax.set_xticks(np.arange(support_ticks.size), support_ticks)
                ax.set_ylabel("window")
                ax.set_yticks(np.arange(window_ticks.size), window_ticks)
                ax.set_zlabel("precision")  # type: ignore
                ax.set_zlim((0, 1))  # type: ignore
            fig.savefig(results_folder / "precision.svg")
        finally:
            close(fig)

        fig = figure(figsize=(8, 3))
        try:
            for type_idx, (type, results) in enumerate(typed_results.items()):
                data = defaultdict[str, list[float]](list)
                for result in results:
                    if result.performance.get("recall", -1) == -1:
                        continue
                    if result.performance["recall"] == 0:
                        # If data size is less than window size, our current implementation of the stream miner will produce no rules.
                        continue
                    for key, val in result.parameters.items():
                        data[key].append(val)
                    data["recall"].append(result.performance["recall"])

                support_ticks, support = np.unique(data["support"], return_inverse=True)
                window_ticks, window = np.unique(
                    data["window"] or -1, return_inverse=True
                )
                zz = data["recall"]
                if not zz:
                    continue

                ax = fig.add_subplot(
                    1, len(typed_results), type_idx + 1, projection="3d"
                )
                ax.bar3d(  # type: ignore
                    support,
                    window,
                    0,
                    1,
                    1,
                    zz,
                    color=np.random.rand(len(zz), 3),
                    shade=True,
                )

                ax.set_title(type)
                ax.set_xlabel("support")
                ax.set_xticks(np.arange(support_ticks.size), support_ticks)
                ax.set_ylabel("window")
                ax.set_yticks(np.arange(window_ticks.size), window_ticks)
                ax.set_zlabel("recall")  # type: ignore
                ax.set_zlim((0, 1))  # type: ignore
            fig.savefig(results_folder / "recall.svg")
        finally:
            close(fig)


if __name__ == "__main__":
    main()
