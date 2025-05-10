import matplotlib.pyplot as plt

io_sizes = [4096, 8192, 16384, 32768, 65536]
fat32_latencies = [5681.04, 5710.27, 5809.77, 6013.31, 6807.38]
ext4_latencies = [6874.37, 6797.77, 7113.19, 7253.12, 7454.21]
xfs_latencies = [5752.94, 5830.60, 5958.48, 5896.05, 6248.07]
f2fs_latencies = [7402.58, 7333.08, 7454.35, 8157.18, 8391.97]

plt.plot(io_sizes, fat32_latencies, label='FAT32', marker='o')
plt.plot(io_sizes, ext4_latencies, label='ext4', marker='o')
plt.plot(io_sizes, xfs_latencies, label='xfs', marker='o')
plt.plot(io_sizes, f2fs_latencies, label='f2fs', marker='o')

plt.xlabel('IO Size (bytes)')
plt.ylabel('Median IO Latency (microseconds)')
plt.title('Random IO Latency vs. IO Size')
plt.legend()
plt.grid(True)
plt.savefig('io_latency.png')
# plt.show()
