# lfu module

LFU_DIRPATH = lib/caches
LFU_INCDIR = -I$(LFU_DIRPATH)/include
INCDIR += $(LFU_INCDIR)

# hacked part
# NOTE: LFU uses template class -> NO need to compile into binary objects and static/dynamic libraries