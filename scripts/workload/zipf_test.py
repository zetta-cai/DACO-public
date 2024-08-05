#!/usr/bin/env python3
# zipf_test: calculate the probabilities of the hottest objects based on power-law Zipf distribution of numpy in python to test the correctness of ZipfWorkloadWrapper in C++.

import numpy as np

def zipf_func(x, a):
    # x is object rank (>= 1) and a is Zipfian constant
    return 1.0/(x**a)

# For Wikipedia text workload
wikitext_datasetcnt = 1000000
wikitext_zipf_constant = 2.4269573520957066
wikitext_perrank_probs = []
for i in range(wikitext_datasetcnt):
    wikitext_perrank_probs.append(zipf_func(i+1, wikitext_zipf_constant))
wikitext_perrank_probs = np.array(wikitext_perrank_probs)
wikitext_perrank_probs = wikitext_perrank_probs / wikitext_perrank_probs.sum()
print("wikitext rank-1 prob: {}; rank-2 prob: {}".format(wikitext_perrank_probs[0], wikitext_perrank_probs[1]))

# For Wikipedia image workload
wikiimage_datasetcnt = 1000000
wikiimage_zipf_constant = TODO
wikiimage_perrank_probs = []
for i in range(wikiimage_datasetcnt):
    wikiimage_perrank_probs.append(zipf_func(i+1, wikiimage_zipf_constant))
wikiimage_perrank_probs = np.array(wikiimage_perrank_probs)
wikiimage_perrank_probs = wikiimage_perrank_probs / wikiimage_perrank_probs.sum()
print("wikiimage rank-1 prob: {}; rank-2 prob: {}".format(wikiimage_perrank_probs[0], wikiimage_perrank_probs[1]))

# # (OBSOLETE due to NOT open-sourced and hence NO total frequency information for probability calculation and curvefitting) For Facebook photo caching workload
# fbphoto_datasetcnt = 1000000
# fbphoto_zipf_constant = 1.67
# fbphoto_perrank_probs = []
# for i in range(fbphoto_datasetcnt):
#     fbphoto_perrank_probs.append(zipf_func(i+1, fbphoto_zipf_constant))
# fbphoto_perrank_probs = np.array(fbphoto_perrank_probs)
# fbphoto_perrank_probs = fbphoto_perrank_probs / fbphoto_perrank_probs.sum()
# print("fbphoto rank-1 prob: {}; rank-2 prob: {}".format(fbphoto_perrank_probs[0], fbphoto_perrank_probs[1]))
