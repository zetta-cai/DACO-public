# lfu module

LFU_DIRPATH = $(LIBRARY_DIRPATH)/caches
LFU_INCDIR = -I$(LFU_DIRPATH)/include
INCDIR += $(LFU_INCDIR)

# hacked part: src/cache/lfu/lfu_cache_policy.hpp
# NOTE: LFU uses template class -> NO need to compile into binary objects and static/dynamic libraries