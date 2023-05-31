# lru module

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

# hacked part
LRU_SRCFILES = $(wildcard $(COVERED_DIRPATH)/cache/cpp-lru-cache/*.c)
LRU_OBJECTS += $(LRU_SRCFILES:.c=.o)
LRU_SHARED_OBJECTS += $(LRU_SRCFILES:.c=.shared.o)
DEPS += $(LRU_SRCFILES:.c=.d)
CLEANS += $(LRU_OBJECTS) $(LRU_SHARED_OBJECTS)