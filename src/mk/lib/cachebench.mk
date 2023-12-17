# cachebench module

CACHEBENCH_DIRPATH := lib/CacheLib

CACHEBENCH_LDDIR := -L$(CACHEBENCH_DIRPATH)/build-cachelib/cachebench -L$(CACHEBENCH_DIRPATH)/opt/cachelib/lib
LDDIR += $(CACHEBENCH_LDDIR)

# Cachelib libs from $(CACHEBENCH_DIRPATH)/build-cachelib/cachebench and $(CACHEBENCH_DIRPATH)/opt/cachelib/lib
CACHEBENCH_LDLIBS := -l:libcachelib_cachebench.a -l:libcachelib_allocator.a -l:libcachelib_datatype.a -l:libcachelib_navy.a -l:libcachelib_shm.a -l:libcachelib_common.a
# Third-party libs from $(CACHEBENCH_DIRPATH)/opt/cachelib/lib
CACHEBENCH_LDLIBS += -l:libfolly.so -l:libfmt.so -l:libzstd.so -l:libglog.so -l:libnuma.so
# Third-party libs from /usr/lib/x86_64-linux-gnu
CACHEBENCH_LDLIBS += -l:libdouble-conversion.so -l:libboost_system.so -l:libpthread.so -lrt
LDLIBS += $(CACHEBENCH_LDLIBS)

CACHEBENCH_INCDIR := -I$(CACHEBENCH_DIRPATH) -I$(CACHEBENCH_DIRPATH)/opt/cachelib/include
INCDIR += $(CACHEBENCH_INCDIR)

# hacked part
CACHEBENCH_SRCFILES = $(wildcard $(COVERED_DIRPATH)/workload/cachebench/*.c)
CACHEBENCH_OBJECTS += $(CACHEBENCH_SRCFILES:.c=.o)
CACHEBENCH_SHARED_OBJECTS += $(CACHEBENCH_SRCFILES:.c=.shared.o)
DEPS += $(CACHEBENCH_SRCFILES:.c=.d)
CLEANS += $(CACHEBENCH_OBJECTS) $(CACHEBENCH_SHARED_OBJECTS)
LINK_OBJECTS += $(CACHEBENCH_OBJECTS)