import matplotlib.pyplot as plt
import pandas as pd

data = pd.read_csv("timings.txt", sep=r"\s+")

processes = [8, 16, 32]
msg_sizes = [262144, 1048576]

box_data = []
labels = []

for P in processes:
    for M in msg_sizes:
        times = data[(data["P"] == P) & (data["M"] == M)]["time"]
        box_data.append(times)
        labels.append(f"P={P}\nM={M}")

plt.figure(figsize=(10, 6))
plt.boxplot(box_data)
plt.xticks(range(1, len(labels) + 1), labels)
plt.xlabel("Processes (P)")
plt.ylabel("Time (seconds)")
plt.title("Execution Time vs Number of Processes")
plt.grid(axis="y")
plt.savefig("plot.png")
plt.show()
