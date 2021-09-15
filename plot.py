import argparse
import json
import os
import re

from dataclasses import dataclass

import matplotlib.pyplot as plt


class BenchmarkResults(object):
    """Represents benchmark results, reported as JSON by google benchmark."""

    def __init__(self, results_file):
        """Constructor that loads the results into this object"""
    
        if not os.path.isfile(results_file):
            raise argparse.ArgumentTypeError(f"Unabled to load: '{results_file}'!")

        with open(results_file, 'r') as db:
            self.__dict__ = json.load(db)

@dataclass
class DataSet(object):
    """POD type for a plottable benchmark"""
    name: str
    strategy: str
    x: list[float]
    y: list[float]

def decompose_name(benchmark_name):
    """Extracts the benchmark name and execution strategy from its run name"""
    match = re.match(r'(\w+)<queries::(\w+)>', benchmark_name)
    return match.groups()

def create_datasets(benchmark_data):
    """Transforms the benchmark data into a dictionary of data sets"""
    datasets = {}
    
    def Map(benchmark):
        """Maps a dictionary representation of a benchmark result to a dataset"""
        name, strategy = decompose_name(benchmark['name'])
        return DataSet(
            name,
            strategy,
            [benchmark['sample_size']],
            [benchmark['items_per_second']]
        )

    def Reduce(aggregate, item):
        """Reduce method, which aggregates x and y values in a dataset"""
        aggregate.x.extend(item.x)
        aggregate.y.extend(item.y)

        return aggregate

    for benchmark in map(Map, benchmark_data):
        key = benchmark.name + benchmark.strategy
        if key in datasets:
            datasets.update({key: Reduce(datasets[key], benchmark)})
        else:
            datasets.update({key: benchmark})
            
    return datasets.values()

def create_and_serialize_chart(iterable, title, file_name):
    """Generates a chart based on the given data and serializes it to disk"""
    fig, axis = plt.subplots()

    lines = []
    for dataset in iterable:
        line, = axis.plot(dataset.x, dataset.y, 'o-', label=dataset.strategy)
        lines.append(line)

    plt.legend(handles=lines)
    axis.set(xscale='log', yscale='log')

    axis.set(xlabel='Sample Size', ylabel='Items / Second', title=title)
    axis.grid()

    fig.savefig(f"{file_name}.png")

def setup_arguments():
    """Configures the acceptable command-line arguments."""

    parser = argparse.ArgumentParser(
        description="Generate line graphs for benchmark results")

    parser.add_argument("results", nargs=1, type=BenchmarkResults,
        help="An absolute or relative path to a .json google benchmarks results file")

    return parser

def main():
    parser = setup_arguments()
    args = parser.parse_args()

    datasets = create_datasets(args.results[0].benchmarks)

    q1_title = 'Least and Most Popular - Throughput'
    q2_title = 'Largest Number of Purchases - Throughput'
    q3_title = 'Number of Transactions over $15 - Throughput'

    def filter_q1(dataset):
        return 'Least' in dataset.name and 'Std' not in dataset.strategy

    def filter_q2(dataset):
        return 'Largest' in dataset.name and 'Std' not in dataset.strategy

    def filter_q3(dataset):
        return '15' in dataset.name and 'Std' not in dataset.strategy

    def filter_q1_no_ia(dataset):
        return filter_q1(dataset) and 'IA' not in dataset.strategy
                           
    def filter_q2_no_ia(dataset):
        return filter_q2(dataset) and 'IA' not in dataset.strategy

    def filter_q3_no_ia(dataset):
        return filter_q3(dataset) and 'IA' not in dataset.strategy

    plt.style.use('seaborn-deep')

    create_and_serialize_chart(filter(filter_q1, datasets), q1_title, 'q1')
    create_and_serialize_chart(filter(filter_q2, datasets), q2_title, 'q2')
    create_and_serialize_chart(filter(filter_q3, datasets), q3_title, 'q3')
    create_and_serialize_chart(filter(filter_q1_no_ia, datasets), q1_title, 'q1_no_ia')
    create_and_serialize_chart(filter(filter_q2_no_ia, datasets), q2_title, 'q2_no_ia')
    create_and_serialize_chart(filter(filter_q3_no_ia, datasets), q3_title, 'q3_no_ia')

if __name__ == "__main__":
    main()
