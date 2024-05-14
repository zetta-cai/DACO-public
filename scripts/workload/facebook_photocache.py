#!/usr/bin/env python3
# (OBSOLETE due to unused) facebook_photocache: generate Facebook photo cache workload

import numpy as np
import struct
from scipy.optimize import curve_fit
from scipy.special import zetac

from ..common import *

def zipf_func(x, a):
    return (x**-a)/zetac(a)

# NOTE: the numbers come from the paper of SOSP'13 An Analysis of Facebook Photo Caching

# (1) For object size distribution (just approximate fitting, as the paper does not mention which distribution the object size belongs to)

# 1%: 1 KiB
# 1%: 1 - 2 KiB
# 18%: 2 - 4 KiB
# 10%: 4 - 8 KiB
# 10%: 8 - 16 KIB
# 5%: 16 - 32 KiB
# 20%: 32 - 64 KiB
# 30%: 64 - 128 KiB
# 4.1%: 128 - 256 KiB
# 0.3%: 256 - 512 KiB
# 0.3%: 512 KiB - 1 MiB
# 0.3%: 1 MiB - 2 MiB

# (2) Dataset statistics

# 1.3M unique objects
# 77M requests -> we ONLY consider 1M requests randomly for evaluation

# (3) For popularity distribution

# Curve fitting by Zipfian distribution
pop_x = np.array([10**0, 10**1, 10**2, 10**3, 10**4, 10**5, 10**6])
pop_y = np.array([10**6, 10**5, 10**4, 10**3.1, 10**2.2, 10**1.2, 10**0])
pop_y = pop_y/pop_y.sum()
result = curve_fit(zipf_func, pop_x, pop_y, p0=[1.1])
zipf_a = result[0][0]

# (4) Generate dataset

np.random.seed(0)

# Generate dataset frequencies
dataset_size = int(1.3*10**6)
zipf_dataset = np.random.zipf(zipf_a, dataset_size)
zipf_dataset = np.sort(zipf_dataset)[::-1] # Descending order

# Convert into dataset keys and probabilities
dataset_keys = np.array(range(1, dataset_size + 1))
dataset_probs = zipf_dataset/zipf_dataset.sum()

# Generate dataset value sizes
dataset_value_sizes = np.zeros(dataset_size)
samples = np.random.uniform(0, 999, size=dataset_size)
for i in range(len(samples)):
    tmp_sample = samples[i]
    if tmp_sample < 10:
        dataset_value_sizes[i] = 1024
    elif tmp_sample < 20:
        dataset_value_sizes[i] = 1024 + (2048 - 1024) * (tmp_sample - 9) / (19 - 9)
    elif tmp_sample < 200:
        dataset_value_sizes[i] = 2048 + (4096 - 2048) * (tmp_sample - 19) / (199 - 19)
    elif tmp_sample < 300:
        dataset_value_sizes[i] = 4096 + (8192 - 4096) * (tmp_sample - 199) / (299 - 199)
    elif tmp_sample < 400:
        dataset_value_sizes[i] = 8192 + (16384 - 8192) * (tmp_sample - 299) / (399 - 299)
    elif tmp_sample < 450:
        dataset_value_sizes[i] = 16384 + (32768 - 16384) * (tmp_sample - 399) / (449 - 399)
    elif tmp_sample < 650:
        dataset_value_sizes[i] = 32768 + (65536 - 32768) * (tmp_sample - 449) / (649 - 449)
    elif tmp_sample < 950:
        dataset_value_sizes[i] = 65536 + (131072 - 65536) * (tmp_sample - 649) / (949 - 649)
    elif tmp_sample < 991:
        dataset_value_sizes[i] = 131072 + (262144 - 131072) * (tmp_sample - 949) / (990 - 949)
    elif tmp_sample < 994:
        dataset_value_sizes[i] = 262144 + (524288 - 262144) * (tmp_sample - 990) / (993 - 990)
    elif tmp_sample < 997:
        dataset_value_sizes[i] = 524288 + (1048576 - 524288) * (tmp_sample - 993) / (996 - 993)
    elif tmp_sample < 1000:
        dataset_value_sizes[i] = 1048576 + (2097152 - 1048576) * (tmp_sample - 996) / (999 - 996)
    else:
        LogUtil.die(Common.scriptname, "invalid tmp_sample {}".format(tmp_sample))

# Dump dataset value size statistics
min_value_size = dataset_value_sizes.min()
max_value_size = dataset_value_sizes.max()
sum_value_size = dataset_value_sizes.sum()
avg_value_size = sum_value_size / dataset_size
dataset_space_usage = dataset_size * 4 + sum_value_size
print("min_value_size: {} B; max_value_size: {} B; avg_value_size: {} B; dataset_space_usage: {} MiB".format(min_value_size, max_value_size, avg_value_size, dataset_space_usage / 1024 / 1024))

# (5) Dump dataset size, probs, and value sizes

# NOTE: MUST be the same as dataset filepath in src/common/util.c
dataset_filepath = "{}/fbphoto.dataset".format(Common.trace_dirpath)

# Dump into the binary file
if not os.path.exists(dataset_filepath):
    print("Dump dataset file...")
    os.makedirs(os.path.dirname(dataset_filepath), exist_ok=True)
    with open(dataset_filepath, "wb") as f:
        # Dump dataset size as uint32_t in little endian
        f.write(struct.pack("<I", dataset_size))
        # Dump probs as float and value sizes as uint32_t in little endian
        for i in range(dataset_size):
            f.write(struct.pack("<f", dataset_probs[i]))
            f.write(struct.pack("<I", int(dataset_value_sizes[i])))









# NOTE: workload generation should be performed in C++ code in runtime and we do not need to dump/load dataset/workload files

# # (5) Generate workloads and dump dataset/workload files

# # Generate workload
# workload_size = int(1*10**6) # 1M requests
# workloads = np.random.choice(dataset_keys, size=workload_size, p=dataset_probs)

# # Generate final dataset keys and value sizes
# final_dataset_keys = np.unique(workloads)
# final_dataset_key_indices = final_dataset_keys - 1
# final_dataset_value_size = dataset_value_sizes[final_dataset_key_indices]

# # TODO: Dump dataset/workload files