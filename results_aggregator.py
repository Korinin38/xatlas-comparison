import pandas as pd
import matplotlib.pyplot as plt
import sys


def load_csv(filename: str):
	df = pd.read_csv(filename)
	df.set_index('Charts', inplace=True)
	df.sort_index(inplace=True)
	return df


def filter_data(a: pd.DataFrame):
	return a[a["Resolution"] == 4096]


def generate_plot(a: pd.DataFrame, output_name: str):
	new = a[a["Old?"] == 0]
	old = a[a["Old?"] == 1]
	plt.plot(old.index, old["Time"], label="old")
	plt.plot(new.index, new["Time"], label="new")
	plt.xlabel("Number of charts")
	plt.ylabel("Time, s")
	plt.legend()
	plt.title("Comparison between new and old aglorithms")
	plt.savefig(output_name)


if __name__ == "__main__":
	a = load_csv(sys.argv[1])
	a = filter_data(a)
	generate_plot(a, sys.argv[2])

