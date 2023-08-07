# lfu module

LFU_DIRPATH = lib/caches
LFU_INCDIR = -I$(LFU_DIRPATH)/include
INCDIR += $(LFU_INCDIR)

# hacked part (LFU uses template class -> NO need to compile into binary objects and static/dynamic libraries)
#LFU_SRCFILES = $(wildcard $(COVERED_DIRPATH)/cache/lfu/*.c)
#LFU_OBJECTS += $(LFU_SRCFILES:.c=.o)
#LFU_SHARED_OBJECTS += $(LFU_SRCFILES:.c=.shared.o)
#DEPS += $(LFU_SRCFILES:.c=.d)
#CLEANS += $(LFU_OBJECTS) $(LFU_SHARED_OBJECTS)