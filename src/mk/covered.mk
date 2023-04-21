COVERED_DIRPATH = src
COVERED_INCDIR = -I$(COVERED_DIRPATH)
INCDIR += $(COVERED_INCDIR)

# benchmark module
BENCHMARK_OBJECTS += $(COVERED_DIRPATH)/benchmark/benchmark_util.o $(COVERED_DIRPATH)/benchmark/client_param.o $(COVERED_DIRPATH)/benchmark/client_wrapper.o $(COVERED_DIRPATH)/benchmark/worker_param.o $(COVERED_DIRPATH)/benchmark/worker.o
BENCHMARK_SHARED_OBJECTS += $(BENCHMARK_OBJECTS:.o=.shared.o)
DEPS += $(BENCHMARK_OBJECTS:.o=.d)
CLEANS += $(BENCHMARK_OBJECTS) $(BENCHMARK_SHARED_OBJECTS)

# common module
COMMON_OBJECTS += $(COVERED_DIRPATH)/common/config.o $(COVERED_DIRPATH)/common/key.o $(COVERED_DIRPATH)/common/param.o $(COVERED_DIRPATH)/common/request.o $(COVERED_DIRPATH)/common/util.o $(COVERED_DIRPATH)/common/value.o
COMMON_SHARED_OBJECTS += $(COMMON_OBJECTS:.o=.shared.o)
DEPS += $(COMMON_OBJECTS:.o=.d)
CLEANS += $(COMMON_OBJECTS) $(COMMON_SHARED_OBJECTS)

# workload module
WORKLOAD_OBJECTS += $(COVERED_DIRPATH)/workload/workload_base.o $(COVERED_DIRPATH)/workload/facebook_workload.o $(COVERED_DIRPATH)/workload/cachebench/workload_generator.o $(COVERED_DIRPATH)/workload/cachebench/cachebench_config.o
WORKLOAD_SHARED_OBJECTS += $(WORKLOAD_OBJECTS:.o=.shared.o)
DEPS += $(WORKLOAD_OBJECTS:.o=.d)
CLEANS += $(WORKLOAD_OBJECTS) $(WORKLOAD_SHARED_OBJECTS)