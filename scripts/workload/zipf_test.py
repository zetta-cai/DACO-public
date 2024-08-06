#!/usr/bin/env python3
# zipf_test: calculate the probabilities of the hottest objects based on power-law Zipf distribution of numpy in python to test the correctness of ZipfWorkloadWrapper in C++.

import numpy as np

def zipf_func(x, a):
    # x is object rank (>= 1) and a is Zipfian constant
    return 1.0/(x**a)

# For Wikipedia text workload
wikitext_datasetcnt = 1000000
wikitext_zipf_constant = 0.7351769996271038
wikitext_perrank_probs = []
for i in range(wikitext_datasetcnt):
    wikitext_perrank_probs.append(zipf_func(i+1, wikitext_zipf_constant))
wikitext_perrank_probs = np.array(wikitext_perrank_probs)
wikitext_perrank_probs = wikitext_perrank_probs / wikitext_perrank_probs.sum()
print("wikitext rank-1 prob: {}; rank-2 prob: {}".format(wikitext_perrank_probs[0], wikitext_perrank_probs[1]))

# For Wikipedia image workload
wikiimage_datasetcnt = 1000000
wikiimage_zipf_constant = 1.0128733530793717
wikiimage_perrank_probs = []
for i in range(wikiimage_datasetcnt):
    wikiimage_perrank_probs.append(zipf_func(i+1, wikiimage_zipf_constant))
wikiimage_perrank_probs = np.array(wikiimage_perrank_probs)
wikiimage_perrank_probs = wikiimage_perrank_probs / wikiimage_perrank_probs.sum()
print("wikiimage rank-1 prob: {}; rank-2 prob: {}".format(wikiimage_perrank_probs[0], wikiimage_perrank_probs[1]))

# For Tencent photo 1 workload
tencentphoto1_datasetcnt = 1000000
tencentphoto1_zipf_constant = 0.6645665328056994
tencentphoto1_perrank_probs = []
for i in range(tencentphoto1_datasetcnt):
    tencentphoto1_perrank_probs.append(zipf_func(i+1, tencentphoto1_zipf_constant))
tencentphoto1_perrank_probs = np.array(tencentphoto1_perrank_probs)
tencentphoto1_perrank_probs = tencentphoto1_perrank_probs / tencentphoto1_perrank_probs.sum()
print("tencentphoto1 rank-1 prob: {}; rank-2 prob: {}".format(tencentphoto1_perrank_probs[0], tencentphoto1_perrank_probs[1]))

# For Tencent photo 2 workload
tencentphoto2_datasetcnt = 1000000
tencentphoto2_zipf_constant = 0.6566908883710469
tencentphoto2_perrank_probs = []
for i in range(tencentphoto2_datasetcnt):
    tencentphoto2_perrank_probs.append(zipf_func(i+1, tencentphoto2_zipf_constant))
tencentphoto2_perrank_probs = np.array(tencentphoto2_perrank_probs)
tencentphoto2_perrank_probs = tencentphoto2_perrank_probs / tencentphoto2_perrank_probs.sum()
print("tencentphoto2 rank-1 prob: {}; rank-2 prob: {}".format(tencentphoto2_perrank_probs[0], tencentphoto2_perrank_probs[1]))

# # (OBSOLETE due to NOT open-sourced and hence NO total frequency information for probability calculation and curvefitting) For Facebook photo caching workload
# fbphoto_datasetcnt = 1000000
# fbphoto_zipf_constant = 1.67
# fbphoto_perrank_probs = []
# for i in range(fbphoto_datasetcnt):
#     fbphoto_perrank_probs.append(zipf_func(i+1, fbphoto_zipf_constant))
# fbphoto_perrank_probs = np.array(fbphoto_perrank_probs)
# fbphoto_perrank_probs = fbphoto_perrank_probs / fbphoto_perrank_probs.sum()
# print("fbphoto rank-1 prob: {}; rank-2 prob: {}".format(fbphoto_perrank_probs[0], fbphoto_perrank_probs[1]))
