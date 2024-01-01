# fifo module

FIFO_DIRPATH = $(LIBRARY_DIRPATH)/caches
FIFO_INCDIR = -I$(FIFO_DIRPATH)/include
INCDIR += $(FIFO_INCDIR)

# hacked part: src/cache/fifo/fifo_cache_policy.hpp
# NOTE: FIFO uses template class -> NO need to compile into binary objects and static/dynamic libraries