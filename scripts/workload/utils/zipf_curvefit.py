#!/usr/bin/env python3
# zipf_curvefit: curvefit the given statistics (sorted frequency list, i.e., rank-frequency pairs) by Zipfian distribution to get Zipfian constant.

import numpy as np
from scipy.optimize import curve_fit
from scipy.special import zetac

from ...common import *

def zipf_func(x, a):
    # x is object rank (>= 1) and a is Zipfian constant (> 1)
    return (x**-a)/zetac(a)

class ZipfCurvefit:

    CURVEFIT_SAMPLE_COUNT = 1 * 1000 * 1000 # 1M samples for curvefit

    # Curvefit the given sorted frequency list to get Zipfian constant
    # NOTE: sample the given rank-frequency pairs to reduce curvefit computation overhead -> just an impl trick and the sampled rank-frequency pairs are still based on the entire trace (instead, using partial trace will get smaller ranks and frequencies for a given sampled object)
    def __init__(self, sorted_frequency_list):
        np.random.seed(0)

        # Generate sampled indices
        sorted_indices_array = np.arange(len(sorted_frequency_list))
        if len(sorted_indices_array) > CURVEFIT_SAMPLE_COUNT:
            sampled_indices_array = np.random.choice(sorted_indices_array, CURVEFIT_SAMPLE_COUNT, replace=False) # Each indice can only be sampled once
        else:
            sampled_indices_array = sorted_indices_array

        # NOTE: NO need to sort sampled indices, as curvefit does NOT require ordered input, i.e., <(x0, y0), <x1, y1>> has the same curve as <(x1, y1), (x0, y0)>
        sampled_ranks_array = sampled_indices_array + 1
        sampled_frequencies_array = np.array(sorted_frequency_list)[sampled_indices_array]

        # Curvefit based on Zipfian distribution
        LogUtil.prompt("Perform curvefitting...")
        curvefit_result = curve_fit(zipf_func, sampled_ranks_array, sampled_frequencies_array, p0=[1.1])
        self.zipf_constant_ = curvefit_result[0][0]
        LogUtil.prompt("Zipfian constant: {}".format(self.zipf_constant_))
    
    # Get Zipfian constant
    def getZipfConstant(self):
        return self.zipf_constant_