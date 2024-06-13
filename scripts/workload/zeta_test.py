#!/usr/bin/env python3
# zeta_test: calculate the probabilities of the hottest objects based on Zeta distribution of numpy in python to test the correctness of ZetaWorkloadWrapper in C++.

import numpy as np
from scipy.special import zetac

def zipf_func(x, a):
    return (x**-a)/zetac(a)

# For Wikipedia text workload
wikitext_datasetcnt = 1000000
wikitext_zipf_constant = 1.0119666595279269
wikitext_perrank_probs = []
for i in range(wikitext_datasetcnt):
    wikitext_perrank_probs.append(zipf_func(i+1, wikitext_zipf_constant))
wikitext_perrank_probs = np.array(wikitext_perrank_probs)
wikitext_perrank_probs = wikitext_perrank_probs / wikitext_perrank_probs.sum()
print("wikitext rank-1 prob: {}; rank-2 prob: {}".format(wikitext_perrank_probs[0], wikitext_perrank_probs[1]))

# For Wikipedia image workload
wikiimage_datasetcnt = 1000000
wikiimage_zipf_constant = 1.0286687302231508
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
