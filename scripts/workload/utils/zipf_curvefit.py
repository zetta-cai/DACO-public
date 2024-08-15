#!/usr/bin/env python3
# zipf_curvefit: curvefit the given statistics (sorted frequency list, i.e., rank-frequency pairs) by power-law Zipfian distribution to get Zipfian constant.

import numpy as np
from scipy.optimize import curve_fit

from ...common import *

# def zipf_func(x, a, c):
#     # x is object rank (>= 1) and a is Zipfian constant (c is a constant scaling factor to convert relative frequency into probability)
#     return 1.0/(x**a) * c

def zipf_func(x, a):
    # x is object rank (>= 1) and a is Zipfian constant
    return 1.0/(x**a) # NOTE: return relative frequency instead of probability!!!

class ZipfCurvefit:

    CURVEFIT_SAMPLE_COUNT = 1 * 1000 * 1000 # 1M samples for curvefit

    # Curvefit the given rank-frequency map to get Zipfian constant
    # NOTE: sample the given rank-frequency pairs to reduce curvefit computation overhead -> just an impl trick and the sampled rank-frequency pairs are still based on the entire trace (instead, using partial trace will get smaller ranks and frequencies for a given sampled object)
    def __init__(self, rank_frequency_map):
        np.random.seed(0)

        # (1) Normalize to get relative frequencies

        # # sorted_probabilities_list = sorted_frequency_list / sorted_frequency_list.sum()
        # sorted_relative_frequency_list = sorted_frequency_list / sorted_frequency_list[0]

        if 1 not in rank_frequency_map:
            LogUtil.die(Common.scriptname, "cannot get relative frequencies if without the frequency of rank-1 object!")
        sorted_ranks_array = [] # Ascending order of ranks
        sorted_relative_frequency_array = [] # Descending order of frequencies
        rank1_frequency = float(rank_frequency_map[1])
        for tmp_rank, tmp_frequency in rank_frequency_map.items(): # rank_frequency_map follows insertion order of rank-frequency pairs (see scripts/workload/utils/trace_loader.py::getRankFrequencyMap())
            sorted_ranks_array.append(tmp_rank)
            sorted_relative_frequency_array.append(float(tmp_frequency) / rank1_frequency)
        if sorted_relative_frequency_array[0] != 1.0:
            LogUtil.die(Common.scriptname, "rank-1 object should have the relative frequency of 1.0 after normalizing!")

        # (2) Sample input if necessary

        # # sorted_indices_array = np.arange(len(sorted_probabilities_list))
        # sorted_indices_array = np.arange(len(sorted_relative_frequency_list))

        # Generate sampled indices
        sorted_indices_array = np.arange(len(sorted_ranks_array))
        if len(sorted_indices_array) > ZipfCurvefit.CURVEFIT_SAMPLE_COUNT:
            LogUtil.dump(Common.scriptname, "sample {} points from {} points...".format(ZipfCurvefit.CURVEFIT_SAMPLE_COUNT, len(sorted_indices_array)))
            sampled_indices_array = np.random.choice(sorted_indices_array, ZipfCurvefit.CURVEFIT_SAMPLE_COUNT, replace=False) # Each indice can only be sampled once
        else:
            sampled_indices_array = sorted_indices_array

        # sampled_ranks_array = sampled_indices_array + 1
        # # sampled_probabilities_array = np.array(sorted_probabilities_list)[sampled_indices_array]
        # sampled_relative_frequency_array = np.array(sorted_relative_frequency_list)[sampled_indices_array]

        # NOTE: NO need to sort sampled indices, as curvefit does NOT require ordered input, i.e., <(x0, y0), <x1, y1>> has the same curve as <(x1, y1), (x0, y0)>
        sampled_ranks_array = np.array(sorted_ranks_array)[sampled_indices_array]
        sampled_relative_frequency_array = np.array(sorted_relative_frequency_array)[sampled_indices_array]

        # (3) Curvefit based on Zipfian distribution

        LogUtil.prompt(Common.scriptname, "Perform curvefitting...")
        # curvefit_result = curve_fit(zipf_func, sampled_ranks_array, sampled_probabilities_array, p0=[1.1, 0.1])
        curvefit_result = curve_fit(zipf_func, sampled_ranks_array, sampled_relative_frequency_array, p0=[1.1])
        self.zipf_constant_ = curvefit_result[0][0]
        # self.zipf_scaling_factor_ = curvefit_result[0][1]
        LogUtil.dump(Common.scriptname, "Zipfian constant: {}".format(self.zipf_constant_))
        # LogUtil.dump(Common.scriptname, "Zipfian scaling factor: {}".format(self.zipf_scaling_factor_))

    # Get Zipfian constant
    def getZipfConstant(self):
        return self.zipf_constant_
    
    # # Get Zipfian scaling factor
    # def getZipfScalingFactor(self):
    #     return self.zipf_scaling_factor_