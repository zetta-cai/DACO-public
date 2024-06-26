from joint_dst import *
from constants import *
from collections import defaultdict
import datetime
import os
import time
import sys

# Siyuan: use a fixed random seed for dataset generation, so that all clients access the same application in geo-distributed settings
import random
import numpy as np

## A class that defines the functions and objects required to generate a synthetic trace
class TraceGenerator():
    # Siyuan: pass output_dir and client_worker_count to dump multi-client-worker workload sequences and dataset file into the given output directory
    def __init__(self, trafficMixer, args, output_dir, client_worker_count, dataset_objcnt, printBox=None):
        self.trafficMixer = trafficMixer
        self.args = args
        self.log_file = open("OUTPUT/logfile.txt" , "w")
        self.read_obj_size_dst()
        self.read_popularity_dst()
        self.curr_iter = 0
        self.printBox = printBox

        # Siyuan: move fixed variables here for global access
        self.fd = self.trafficMixer.FD_mix
        self.OWV = self.trafficMixer.weight_vector ## object weight vector
        self.trafficClasses = self.trafficMixer.trafficClasses        

        # Siyuan: use 1M dataset objects to generate workloads for fair comparison
        # self.dataset_objcnt = 70*MIL # Original settings of TRAGEN
        self.dataset_objcnt = dataset_objcnt # Use 1M by default (see scripts/tools/gen_akamai_traces.py)

        # Siyuan: save output directory and client worker count to dump multi-client-worker workload sequences into the given output directory
        self.output_dir = output_dir
        self.client_worker_count = client_worker_count
        

    ## Generate a synthetic trace
    def generate(self):
        # Siyuan: use a fixed random seed for dataset generation, so that all clients access the same application in geo-distributed settings
        random.seed(0)
        np.random.seed(0)

        ## sample 70 million objects
        print("Sampling the object sizes that will be assigned to the initial objects in the LRU stack ...")

        if self.printBox != None:
            self.printBox.setText("Sampling initial objects ...")
        
        n_sizes        = []
        sizes          = []
        n_popularities = []
        popularities   = []
        i = -1
        for i in range(len(self.OWV)-1):
            SZ = self.sz_dsts[self.trafficClasses[i]]
            n_sizes.extend(SZ.sample_keys(int(self.dataset_objcnt * self.OWV[i])))

            P = self.popularity_dsts[self.trafficClasses[i]]
            n_popularities.extend(P.sample_keys(int(self.dataset_objcnt * self.OWV[i])))
            
        SZ = self.sz_dsts[self.trafficClasses[i+1]]
        n_sizes.extend(SZ.sample_keys(int(self.dataset_objcnt) - len(n_sizes)))
        P = self.popularity_dsts[self.trafficClasses[i+1]]
        n_popularities.extend(P.sample_keys(int(self.dataset_objcnt) - len(n_popularities)))
        
        random.shuffle(n_sizes)
        random.shuffle(n_popularities)        
        sizes.extend(n_sizes)        
        popularities.extend(n_popularities)

        # Siyuan: add dataset_total_size to remove stack distances exceeding dataset_total_size of all dataset objects (reasonable as stack distance will never exceed dataset total size in practice)
        dataset_total_size = 0
        for tmp_size in sizes:
            dataset_total_size += tmp_size
        print("dataset_total_size: {}".format(dataset_total_size))
        self.fd.setupSampling(self.args.hitrate_type, 0, TB, dataset_total_size, int(self.dataset_objcnt))
        self.MAX_SD = self.fd.sd_keys[-1]

        # Siyuan: move intermediate files here to avoid overwriting
        debug = open("OUTPUT/debug.txt", "w") ## debug file
        
        # Siyuan: generate trace for each client worker
        for client_worker_idx in range(int(self.client_worker_count)):
            self.generate_for_client_worker(client_worker_idx, sizes, popularities, debug)
        
        # Siyuan: dump dataset file
        dataset_filepath = os.path.join(self.output_dir, "dataset{}.txt".format(self.dataset_objcnt)) # E.g., data/akamai/dataset1000000.txt
        if os.path.exists(dataset_filepath):
            print("dataset file {} already exists!".format(dataset_filepath))
        else:
            dataset_filedesc = open(dataset_filepath, "w")
            for tmp_objid in range(int(self.dataset_objcnt)):
                dataset_filedesc.write(str(tmp_objid) + "," + str(sizes[tmp_objid]) + "\n")
            dataset_filedesc.close()

        # Siyuan: close intermediate files after workload generation
        debug.close()


    # Siyuan: encapsulate for multi-client-worker traces
    def generate_for_client_worker(self, client_worker_idx, sizes, popularities, debug):
        print("Generate trace for client worker {}".format(client_worker_idx))

        # Siyuan: use a unique random seed for each client worker, so that all clients have different workload sequences in geo-distributed settings (yet still access the same dataset)
        workload_rng = random.Random(client_worker_idx) # Determine the sampled stack distances, which affects the workload items by pctile and the nodes deleted from st_tree

        # Siyuan: we continuously generate workload items based on the same dataset into total_trace
        total_trace   = []

        # Siyuan: generate until achieving required trace length
        while len(total_trace) < int(self.args.length):
            remaining_trace_length = int(self.args.length) - len(total_trace)
            # Siyuan: generate different workload sequences under the same dataset
            tmp_trace = self.generate_helper(workload_rng, remaining_trace_length, sizes, popularities, debug)
            total_trace.extend(tmp_trace)

        tm_now = int(time.time())

        # Siyuan: dump workload trace file of current client worker into output_dir instead of random directory
        #os.mkdir("OUTPUT/" + str(tm_now))
        #f = open("OUTPUT/" + str(tm_now) + "/gen_sequence.txt", "w")
        workload_filename = "worker{}_sequence.txt".format(client_worker_idxs)
        workload_dirpath = os.path.join(self.output_dir, "dataset{}_workercnt{}".format(self.dataset_objcnt, self.client_worker_count)) # E.g., data/akamai/dataset1000000_workercnt4/
        workload_filepath = os.path.join(workload_dirpath, workload_filename)
        f = open(workload_filepath, "w")

        with open("OUTPUT/" + str(tm_now) + "/command.txt", 'w') as fp:
            fp.write('\n'.join(sys.argv[1:]))
            
        ## Assign timestamp based on the byte-rate of the FD
        self.assign_timestamps(total_trace, sizes, self.fd.byte_rate, f) # Siyuan: this function will dump workload trace file of the current client worker

        ## We are done!
        if self.printBox != None:
            self.printBox.setText("Done! Ready again ...")
        
        # Siyuan: close the dumped file
        f.close()
    
    def generate_helper(self, workload_rng, remaining_trace_length, sizes, popularities, debug):
        ## Now fill the objects such that the stack is 10TB
        total_sz = 0
        total_objects = 0
        #i = 0 # Siyuan: NO need
        while total_sz < self.MAX_SD: # Siyuan: self.MAX_SD must < dataset_total_size
            total_sz += sizes[total_objects]
            total_objects += 1
            if total_objects % 100000 == 0:
                print("Initializing the LRU stack ... ", int(100 * float(total_sz)/self.MAX_SD), "% complete")

                if self.printBox != None:
                    self.printBox.setText("Initializing the LRU stack ... " + str(int(100 * float(total_sz)/self.MAX_SD)) + "% complete")

        ## build the LRU stack
        trace = range(total_objects)

        ## Represent the objects in LRU stack as leaves of a B+Tree
        trace_list, ss = gen_leaves(trace, sizes, self.printBox)
                            
        ## Construct the tree
        st_tree, lvl = generate_tree(trace_list, self.printBox)
        root = st_tree[lvl][0]
        root.is_root = True
        curr = st_tree[0][0]

        ## Initialize
        c_trace   = []
        tries     = 0
        i         = 0
        j         = 0
        k         = 0
        no_desc   = 0
        fail      = 0
        curr_max_seen = 0

        stack_samples = self.fd.sample(1000, rng = workload_rng)

        sz_added   = 0
        sz_removed = 0
        evicted_   = 0

        reqs_seen   = [0] * int(self.dataset_objcnt) # Siyuan: track # of seen requests for each dataset object
        sizes_seen  = []
        sds_seen    = []
        sampled_fds = []

        if self.printBox != None:
            self.printBox.setText("Generating synthetic trace ...")
        
        # Siyuan: generate remaining_trace_length workload items for the same dataset items (yet different sequences)
        while curr != None and i <= int(remaining_trace_length):

            ## Generate 1000 samples -- makes the computation faster
            if k >= 1000:
                stack_samples = self.fd.sample(1000, rng = workload_rng)
                k = 0

            sd = stack_samples[k]
            k += 1


            ## Rework this part -- can be made much more efficient

            req_objects = []
            no_objects  = 0
            present     = curr
            while no_objects < 50:
                req_objects.append((popularities[present.obj_id] - reqs_seen[present.obj_id], present))
                no_objects += 1
                present = present.findNext()[0]

            if sd < 0:
                pctile = 0
            else:
                pctile = len(req_objects) - int(self.fd.findPr(sd) * len(req_objects)) - 1
                if pctile < 0:
                    pctile = 0
                
            req_objects = sorted(req_objects, key= lambda x:x[0])
            req_obj = req_objects[pctile][1]
            req_count = req_objects[pctile][0]
            # if sd > 0:
            #     while req_count <= 1:
            #         pctile += 1
            #         req_obj = req_objects[pctile][1]
            #         req_count = req_objects[pctile][0]
            
            end_object = False
            if sd < 0:
                ## Introduce a new object
                end_object = True
                sz_removed += req_obj.s
                evicted_ += 1
            else:
                sd = random.randint(sd, sd+200000)

            if sd >= root.s:
                fail += 1
                continue
            
            n  = node(req_obj.obj_id, req_obj.s)        
            n.set_b()            

            ## Add the object at the top of the list to the trace
            c_trace.append(n.obj_id)        

            if req_obj.obj_id > curr_max_seen:
                curr_max_seen = req_obj.obj_id
            
            sampled_fds.append(sd)

            if end_object == False:

                try:
                    ## Insert the object at the specified stack distance
                    descrepency, land, o_id = root.insertAt(sd, n, 0, curr.id, debug)                            
                except:
                    print("sd : ", sd, root.s)
                
                local_uniq_bytes = 0

                if n.parent != None :
                    ## Rebalance the tree if the number of children of the parent node is greater than threshold
                    root = n.parent.rebalance(debug)

            else:
                
                ## As we remove objects from the top of the list, add objects towards the end of the list
                ## so that the we have enough objects in the list i.e., size of the list is greater than MAX_SD
                is_stop = False # Siyuan: used to directly return c_trace for stop condition
                while root.s < self.MAX_SD: # Siyuan: self.MAX_SD must < dataset_total_size

                    ## Require more objects
                    if (total_objects + 1) % (int(self.dataset_objcnt)) == 0:
                        # Siyuan: directly return c_trace instead of generating more dataset objects, such that we can continuously generate remaining workload items for the same dataset objects
                        is_stop = True
                        break

                        # n_sizes        = []
                        # n_popularities = []
                        # i = -1
                        # for i in range(len(self.OWV)-1):
                        #     SZ = self.sz_dsts[self.trafficClasses[i]]
                        #     n_sizes.extend(SZ.sample_keys(int(self.dataset_objcnt * self.OWV[i])))

                        #     P = self.popularity_dsts[self.trafficClasses[i]]
                        #     n_popularities.extend(P.sample_keys(int(self.dataset_objcnt * self.OWV[i])))
            
                        # SZ = self.sz_dsts[self.trafficClasses[i+1]]
                        # n_sizes.extend(SZ.sample_keys(int(self.dataset_objcnt) - len(n_sizes)))
                        # P = self.popularity_dsts[self.trafficClasses[i+1]]
                        # n_popularities.extend(P.sample_keys(int(self.dataset_objcnt) - len(n_popularities)))
        
                        # random.shuffle(n_sizes)
                        # random.shuffle(n_popularities)        
                        # sizes.extend(n_sizes)        
                        # popularities.extend(n_popularities)
                        
                        # reqs_seen_n = [0]*int(self.dataset_objcnt)
                        # reqs_seen.extend(reqs_seen_n)

                        
                    total_objects += 1
                    sz = sizes[total_objects]
                    sz_added += sz
                    n = node(total_objects, sz)
                    n.set_b()
                    
                    ## Insert the new object at the end of the list 
                    descrepency, x, y = root.insertAt(root.s - 1, n, 0, curr.id, debug)
            
                    if n.parent != None:
                        root = n.parent.rebalance(debug)
                
                # Siyuan: break to return c_trace for stop condition
                if is_stop:
                    break

            if curr.obj_id == req_obj.obj_id:
                next, success = curr.findNext()
                while (next != None and next.b == 0) or success == -1:
                    next, success = next.findNext()
                curr = next

            del_nodes = req_obj.cleanUpAfterInsertion(sd, n, debug)        

            if i % 100000 == 0:
                self.log_file.write("Trace computed : " +  str(i) + " " +  str(datetime.datetime.now()) +  " " + str(root.s) + " " + str(total_objects) + " " + str(curr_max_seen) + " fail : " + str(fail) + " sz added : " + str(sz_added) + " sz_removed : " + str(sz_removed) + "\n")
                print("Trace computed : " +  str(i) + " " +  str(datetime.datetime.now()) +  " " + str(root.s) + " " + str(total_objects) + " " + str(curr_max_seen) + " fail : " + str(fail) + " sz added : " + str(sz_added) + " sz_removed : " + str(sz_removed) + " evicted : " +  str(evicted_))
                self.log_file.flush()
                if self.printBox != None:
                    self.printBox.setText("Generating synthetic trace: " + str(i*100/self.args.length) + "% complete ...")
                self.curr_iter = i

            reqs_seen[req_obj.obj_id] += 1
            i += 1
        
        # TODO: Comment debug information
        print("c_trace[0] (objid: {}, objsize: {}) for remaining_trace_length {}".format(c_trace[0], sizes[c_trace[0]], remaining_trace_length))
        
        # Siyuan: return c_trace for total_trace.extend()
        return c_trace

    ## Assign timestamp based on the byte-rate of the FD
    def assign_timestamps(self, c_trace, sizes, byte_rate, f):
        timestamp = 0
        KB_added = 0
        KB_rate = byte_rate/1000

        for c in c_trace:
            KB_added += sizes[c]
            f.write(str(timestamp) + "," + str(c) + "," + str(sizes[c]) + "\n")

            if KB_added >= KB_rate:
                timestamp += 1
                KB_added = 0
            

    ## Read object size distribution of the required traffic classes
    def read_obj_size_dst(self):
        self.sz_dsts = defaultdict()

        for c in self.trafficMixer.trafficClasses:
            sz_dst = SZ_dst("FOOTPRINT_DESCRIPTORS/" + str(c) + "/sz.txt", 0, TB)
            self.sz_dsts[c] = sz_dst


    ## Read object size distribution of the required traffic classes
    def read_popularity_dst(self):
        self.popularity_dsts = defaultdict()

        for c in self.trafficMixer.trafficClasses:
            popularity_dst = POPULARITY_dst("FOOTPRINT_DESCRIPTORS/" + str(c) + "/popularity.txt", 0, TB)
            self.popularity_dsts[c] = popularity_dst

