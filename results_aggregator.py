import pandas as pd
import matplotlib.pyplot as plt
import sys


def load_csv(filename: str):
	df = pd.read_csv(filename)
	df.set_index('Charts', inplace=True)
	df.sort_index(inplace=True)
	return df


def filter_data(df: pd.DataFrame):
	return df[df["Resolution"] == 4096]


def generate_plot(df: pd.DataFrame, output_name: str):
	new = df[df["Old?"] == 0]
	old = df[df["Old?"] == 1]
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

