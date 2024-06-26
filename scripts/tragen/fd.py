from collections import defaultdict
from random import choices
from FDUtils import *


TB = 1000000000

class FD():
    def __init__(self, sd_limit=10*TB, iat_limit=4000000):

        self.sd_limit  = sd_limit
        self.iat_limit = iat_limit

        self.fd_util = FDUtils()
        
        self.no_reqs       = 0
        self.total_bytes   = 0
        self.start_tm      = 0
        self.end_tm        = 0
        self.requests_miss  = 0
        self.bytes_miss     = 0
        self.st            = defaultdict(lambda : defaultdict(float))
        
    def read_from_file(self, f, iat_gran, sd_gran):
        
        l = f.readline()
        l = l.strip().split(" ")

        self.no_reqs      = int(l[0])
        self.total_bytes  = float(l[1])
        self.start_tm     = int(l[2])
        self.end_tm       = int(l[3])
        self.requests_miss = int(l[4])
        self.bytes_miss    = float(l[5])
        self.req_rate     = self.no_reqs/(self.end_tm - self.start_tm)
        self.byte_rate    = self.total_bytes/(self.end_tm - self.start_tm)
        
        for l in f:
            l   = l.strip().split(" ")
            iat = (float(l[0]) // iat_gran) * iat_gran
            sd  = (float(l[1]) // sd_gran) * sd_gran
            pr  = float(l[2])
            self.st[iat][sd] += pr

        self.iat_gran = iat_gran
        self.sd_gran  = sd_gran 
            
    ## convolve oneself with fd2 and store result in fd_res
    def addition(self, fd2, fd_res, hitrate_type):

        print("Computing the traffic model for the traffic mix")
        
        if hitrate_type == "rhr":
            rate1 = self.req_rate
            rate2 = fd2.req_rate
        else:
            rate1 = self.byte_rate
            rate2 = fd2.byte_rate
            
        self.fd_util.convolve_2d_fft(self.st, fd2.st, fd_res.st, rate1, rate2, self.sd_gran)
        fd_res.no_reqs       = self.no_reqs + fd2.no_reqs
        fd_res.total_bytes   = self.total_bytes + fd2.total_bytes
        fd_res.start_tm      = min(self.start_tm, fd2.start_tm)
        fd_res.end_tm        = max(self.end_tm, fd2.end_tm)
        fd_res.requests_miss = self.requests_miss + fd2.requests_miss
        fd_res.bytes_miss    = self.bytes_miss + fd2.bytes_miss
        fd_res.req_rate      = self.req_rate + fd2.req_rate
        fd_res.byte_rate     = self.byte_rate + fd2.byte_rate
        fd_res.shave_off_tail()

        fd_res.iat_gran = self.iat_gran
        fd_res.sd_gran  = self.sd_gran

        
    def shave_off_tail(self):
        pr = 0
        tail = []
        for t in self.st:
            for s in self.st[t]:
                if s > self.sd_limit or t > self.iat_limit:
                    pr += self.st[t][s]
                    tail.append((t,s))
        for (t,s) in tail:
            del self.st[t][s]
            
        self.requests_miss += pr*self.no_reqs
        self.bytes_miss    += pr*self.total_bytes
                    
    
    def scale(self, scale_factor, iat_gran):
        
        self.no_reqs       *= scale_factor
        self.total_bytes   *= scale_factor
        self.requests_miss *= scale_factor
        self.bytes_miss    *= scale_factor
        self.req_rate      *= scale_factor
        self.byte_rate     *= scale_factor

        st_sub = defaultdict(lambda : defaultdict(float))
        
        for iat in self.st.keys():
            t = float(iat)/scale_factor
            t = (float(t) // iat_gran) * iat_gran
            for sd in self.st[iat].keys():
                st_sub[t][sd] += self.st[iat][sd]
        self.st = st_sub

    # Siyuan: add dataset_total_size to avoid sampling stack distance exceeding dataset_total_size of all dataset objects (reasonable as stack distance will never exceed dataset total size in practice)
    def setupSampling(self, hr_type, min_val, max_val, dataset_total_size, dataset_objcnt):
        self.sd_keys = []
        self.sd_vals = []

        SD = defaultdict(lambda :0)
        if hr_type == "bhr":
            SD[-1] = float(self.bytes_miss)/self.total_bytes
        else:
            SD[-1] = float(self.requests_miss)/self.no_reqs

        for t in self.st:
            for s in self.st[t]:
                SD[s] += self.st[t][s]

        self.sd_keys = list(SD.keys())
        self.sd_keys.sort()

        # Siyuan: choose the smaller one
        original_maxsd = self.sd_keys[-1] # max sd for 70M dataset
        scaled_maxsd = original_maxsd * dataset_objcnt / (70 * 1000 * 1000)
        if scaled_maxsd > original_maxsd:
            scaled_maxsd = original_maxsd
        sd_limitation = scaled_maxsd
        if dataset_total_size < scaled_maxsd:
            sd_limitation = dataset_total_size
        print("sd_limitation: {}".format(sd_limitation))

        # Siyuan: delay self.sd_pr later
        #self.sd_pr = defaultdict()
        #curr_pr    = 0 
        for sd in self.sd_keys:
            self.sd_vals.append(SD[sd])
            #curr_pr += SD[sd]
            #if sd >= 0:
            #    self.sd_pr[sd] = float(curr_pr - SD[-1])/(1 - SD[-1])

        # Siyuan: remove sd > sd_limitation and scale probabilities
        # Remove sd > sd_limitation
        last_sd_idx = len(self.sd_keys) - 1
        for i in range(len(self.sd_keys)):
            if self.sd_keys[i] > sd_limitation:
                last_sd_idx = i - 1
                break
        if last_sd_idx < 0:
            print("Error: NO stack distance < sd_limitation {}".format(sd_limitation))
            exit(-1)
        elif last_sd_idx < (len(self.sd_keys) - 1):
            self.sd_keys = self.sd_keys[0:last_sd_idx]
            if self.sd_keys[0] != -1:
                print("Error: self.sd_keys[0] {} != -1".format(self.sd_keys[0]))
            # Scale per-sd probability
            self.sd_vals = self.sd_vals[0:last_sd_idx]
            if self.sd_vals[0] != SD[-1]:
                print("Error: self.sd_vals[0] {} != SD[-1] {}".format(self.sd_vals[0], SD[-1]))
            sd_val_sum = 0
            for i in range(1, len(self.sd_vals)): # NOTE: NOT consider SD[-1], i.e., one-hit-wonder without reuse subsequence
                sd_val_sum += self.sd_vals[i]
            for i in range(1, len(self.sd_vals)): # NOTE: NOT consider SD[-1], i.e., one-hit-wonder without reuse subsequence
                self.sd_vals[i] /= sd_val_sum
        # Calculate cumulative probabilities
        self.sd_pr = defaultdict()
        curr_pr    = 0 
        for sd in self.sd_keys:
            curr_pr += SD[sd]
            if sd >= 0: # NOTE: NOT consider SD[-1], i.e., one-hit-wonder without reuse subsequence
                self.sd_pr[sd] = float(curr_pr - SD[-1])/(1 - SD[-1])
        
        print("# of sds: {}".format(len(self.sd_keys)))
        
        # # Dump debug information
        # for i in range(len(self.sd_keys)):
        #     print("sd {} with probability {}".format(self.sd_keys[i], self.sd_vals[i]))
        # exit(-1)
            
        # print("Finished reading the input models")

            
    def sample(self, n, rng = None):
        if rng is None:
            return choices(self.sd_keys, weights=self.sd_vals, k=n)
        else:
            return rng.choices(self.sd_keys, weights=self.sd_vals, k=n)

    
    def findPr(self, sd):
        return self.sd_pr[sd]

    
