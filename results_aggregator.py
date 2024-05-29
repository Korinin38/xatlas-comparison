import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys


def load_csv(filename: str):
	df = pd.read_csv(filename)
	df.set_index('Charts', inplace=True)
	df.sort_index(inplace=True)
	return df


def filter_data(df: pd.DataFrame):
	# return df[(df["Resolution"] == 8192) & ((df["Old?"] == 1) & (df["BruteForce?"] == 1) | (df["Old?"] == 0) & (df["BruteForce?"] == 0))]
	# return df[(df["Resolution"] == 8192) & (df["BruteForce?"] == 1)]
	return df[(df["Resolution"] == 8192)]


def generate_plot(df: pd.DataFrame, output_name: str):
	new = df[df["Old?"] == 0]
	old = df[df["Old?"] == 1]

	x = np.arange(len(old.index))

	plt.clf()
	plt.figure(figsize=(20, 5))
	plt.grid(which='major', axis='y')

	old_better_time = np.where((old["Time"] < new["Time"]), old["Time"], 0)
	plt.bar(x, old["Time"], label="old", color="tab:orange")
	plt.bar(x, new["Time"], label="new", color="tab:blue")
	plt.bar(x, old_better_time, color="tab:orange")
	plt.ylabel("Время, с")
	plt.ylim(1, 100000)
	plt.yscale('log')

	plt.xticks([])
	plt.legend(loc="upper left")
	plt.xlabel("Сортировка по возрастанию количества чартов")
	plt.title("Сравнение старого и нового алгоритмов")
	plt.savefig(output_name + '.png')


def generate_plot_PE(df: pd.DataFrame, output_name: str):
	new = df[df["Old?"] == 0]
	old = df[df["Old?"] == 1]

	x = np.arange(len(old.index))

	plt.clf()
	plt.figure(figsize=(20, 5))
	plt.grid(which='major', axis='y')

	old_worse_PE = np.where((old["PE"] < new["PE"]), old["PE"], 0)
	plt.bar(x, old["PE"], label="old", color="tab:orange")
	plt.bar(x, new["PE"], label="new", color="tab:blue")
	plt.bar(x, old_worse_PE, color="tab:orange")
	plt.ylim(0, 100)
	plt.ylabel("Плотность упаковки, %")

	plt.xticks([])
	plt.legend(loc="upper left")
	plt.xlabel("Сортировка по возрастанию количества чартов")
	plt.title("Сравнение старого и нового алгоритмов")
	plt.savefig(output_name + '_PE.png')


if __name__ == "__main__":
	a = load_csv(sys.argv[1])
	a = filter_data(a)
	generate_plot_time(a, sys.argv[2])
	generate_plot_PE(a, sys.argv[2])

