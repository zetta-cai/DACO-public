import sys
from arg_util import *
from traffic_mixer import *
from trace_generator import *

if __name__ == "__main__":

    ## get arguments for TRAGEN
    parser = define_arguments()
    params = parser.parse_args()
    
    ## print available footprint descriptors and their default traffic volume
    if params.available_fds == True:
        show_available_fds()
        sys.exit()
    elif params.example == True:
        show_example()
        sys.exit()
    
    # Siyuan: check Akamai's trace existence for current client worker count
    current_akamai_trace_dirpath = os.path.join(params.output_dir, "dataset{}_workercnt{}".format(params.dataset_objcnt, params.client_worker_count)) # E.g., data/akamai/dataset1000000_workercnt4/
    if os.path.exists(current_akamai_trace_dirpath):
        # TODO: Uncomment and remove pass
        # print("Akamai's CDN trace of {} client workers has been generated into {}!".format(tmp_client_worker_count, current_akamai_trace_dirpath))
        # sys.exit()
        pass
    else:
        os.makedirs(current_akamai_trace_dirpath)

    ## Read the config  file
    args   = read_config_file(params.config_file)
        
    ## generate representive footprint descriptor and object weight vector
    ## for the specified traffic mix.
    trafficMixer = TrafficMixer(args)

    ## generate a synthetic trace from the above computed inputs
    traceGenerator = TraceGenerator(trafficMixer, args, params.output_dir, params.client_worker_count, params.dataset_objcnt) # Siyuan: pass output_dir and client_worker_count to generate multi-client workload sequences and the dataset file into the given output directory
    traceGenerator.generate()

    
    
    
