# cachebench module

CACHEBENCH_DIRPATH := lib/CacheLib

CACHEBENCH_LDDIR := -L$(CACHEBENCH_DIRPATH)/build-cachelib/cachebench -L$(CACHEBENCH_DIRPATH)/opt/cachelib/lib
# Cachelib libs from $(CACHEBENCH_DIRPATH)/build-cachelib/cachebench and $(CACHEBENCH_DIRPATH)/opt/cachelib/lib
CACHEBENCH_LDLIBS := -l:libcachelib_cachebench.a -l:libcachelib_allocator.a -l:libcachelib_common.a -l:libcachelib_datatype.a -l:libcachelib_navy.a -l:libcachelib_shm.a
# Third-party libs from $(CACHEBENCH_DIRPATH)/opt/cachelib/lib
CACHEBENCH_LDLIBS += -l:libfolly.so -l:libfmt.so -l:libzstd.so -l:libglog.so -l:libnuma.so

CACHEBENCH_LDDIR += -L/usr/lib/x86_64-linux-gnu
# Third-party libs from /usr/lib/x86_64-linux-gnu
CACHEBENCH_LDLIBS += -l:libdouble-conversion.so -l:libboost_system.so -l:libpthread.so

LDDIR += $(CACHEBENCH_LDDIR)
LDLIBS += $(CACHEBENCH_LDLIBS)

CACHEBENCH_INCDIR := -I$(CACHEBENCH_DIRPATH) -I$(CACHEBENCH_DIRPATH)/opt/cachelib/include
INCDIR += $(CACHEBENCH_INCDIR)

#CACHEBENCH_OBJECTS :=

## objects used by covered
#CACHEBENCH_OBJECTS += $(CACHEBENCH_DIRPATH)/cachelib/cachebench/util/Config.o $(CACHEBENCH_DIRPATH)/cachelib/#cachebench/util/Request.o
#CACHEBENCH_OBJECTS += $(CACHEBENCH_DIRPATH)/cachelib/cachebench/workload/GeneratorBase.o $(CACHEBENCH_DIRPATH)/#cachelib/cachebench/workload/PieceWiseReplayGenerator.o $(CACHEBENCH_DIRPATH)/cachelib/cachebench/workload/KVReplayGenerator.o $(CACHEBENCH_DIRPATH)/cachelib/cachebench/workload/WorkloadGenerator.o $(CACHEBENCH_DIRPATH)/#cachelib/cachebench/workload/OnlineGenerator.o

## objects relied by used ones
#CACHEBENCH_OBJECTS += $(CACHEBENCH_DIRPATH)/cachelib/cachebench/util/CacheConfig.o

#DEPS += $(CACHEBENCH_OBJECTS:.o=.d)
#CLEANS += $(CACHEBENCH_OBJECTS)