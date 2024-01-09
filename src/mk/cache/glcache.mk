# glcache module

# hacked part (include/link path and libs)

GLCACHE_DIRPATH = $(COVERED_DIRPATH)/cache/glcache/micro-implementation/build
GLCACHE_INCDIR = -I$(GLCACHE_DIRPATH)/include
INCDIR += $(GLCACHE_INCDIR)

GLCACHE_LDDIR = -L$(GLCACHE_DIRPATH)/lib
LDDIR += $(GLCACHE_LDDIR)

GLCACHE_LDDIBS = -l:liblibCacheSim.so
LDLIBS += $(GLCACHE_LDDIBS)