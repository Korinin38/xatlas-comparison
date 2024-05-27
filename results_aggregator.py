import pandas as pd
import matplotlib.pyplot as plt
import sys


def load_csv(filename: str):
	df = pd.read_csv(filename)
	df.set_index('Charts', inplace=True)
	df.sort_index(inplace=True)
	return df


if __name__ == "__main__":
	a = load_csv(sys.argv[1])
	# a = a["Time"]
	print(a)
	print(a.index)
	# a.reset_index().plot.scatter(x="Charts", y="Time")
	new = a[(a["Old?"] == 0) & (a["Resolution"] == 8192)]
	old = a[(a["Old?"] == 1) & (a["Resolution"] == 8192)]
	plt.plot(old.index, old["Time"], label="old")
	plt.plot(new.index, new["Time"], label="new")
	plt.xlabel("Number of charts")
	plt.ylabel("Time, s")
	plt.legend()
	plt.title("Comparison between new and old aglorithms")
	plt.savefig(sys.argv[2])
