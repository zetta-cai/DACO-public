# cachebench module

CACHEBENCH_DIRPATH := lib/CacheLib

CACHEBENCH_LDDIR := -L$(CACHEBENCH_DIRPATH)/opt/cachelib/lib $(CACHEBENCH_DIRPATH)/build-cachelib/cachebench
LDDIR += $(CACHEENCH_LDDIR)
CACHEBENCH_LDLIBS := -lcachelib_cachebench
LDLIBS += $(CACHEBENCH_LDLIBS)

CACHEBENCH_INCDIR := -I$(CACHEBENCH_DIRPATH)/opt/cachelib/include -I$(CACHEBENCH_DIRPATH)
INCDIR += $(CACHEBENCH_INCDIR)

#CACHEBENCH_OBJECTS :=

## objects used by covered
#CACHEBENCH_OBJECTS += $(CACHEBENCH_DIRPATH)/cachelib/cachebench/util/Config.o $(CACHEBENCH_DIRPATH)/cachelib/#cachebench/util/Request.o
#CACHEBENCH_OBJECTS += $(CACHEBENCH_DIRPATH)/cachelib/cachebench/workload/GeneratorBase.o $(CACHEBENCH_DIRPATH)/#cachelib/cachebench/workload/PieceWiseReplayGenerator.o $(CACHEBENCH_DIRPATH)/cachelib/cachebench/workload/KVReplayGenerator.o $(CACHEBENCH_DIRPATH)/cachelib/cachebench/workload/WorkloadGenerator.o $(CACHEBENCH_DIRPATH)/#cachelib/cachebench/workload/OnlineGenerator.o

## objects relied by used ones
#CACHEBENCH_OBJECTS += $(CACHEBENCH_DIRPATH)/cachelib/cachebench/util/CacheConfig.o

#DEPS += $(CACHEBENCH_OBJECTS:.o=.d)
#CLEANS += $(CACHEBENCH_OBJECTS)