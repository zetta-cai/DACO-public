# lru module

# hacked part
LRU_SRCFILES = $(wildcard $(COVERED_DIRPATH)/cache/lru/*.c)
LRU_OBJECTS += $(LRU_SRCFILES:.c=.o)
LRU_SHARED_OBJECTS += $(LRU_SRCFILES:.c=.shared.o)
DEPS += $(LRU_SRCFILES:.c=.d)
CLEANS += $(LRU_OBJECTS) $(LRU_SHARED_OBJECTS)
LINK_OBJECTS += $(LRU_OBJECTS)