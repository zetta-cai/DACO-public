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

    ## Read the config  file
    args   = read_config_file(params.config_file)
        
    ## generate representive footprint descriptor and object weight vector
    ## for the specified traffic mix.
    trafficMixer = TrafficMixer(args)

    ## generate a synthetic trace from the above computed inputs
    traceGenerator = TraceGenerator(trafficMixer, args, params.output_dir, params.client_worker_count) # Siyuan: pass output_dir and client_worker_count to generate multi-client workload sequences into the given output directory
    traceGenerator.generate()

    
    
    
